/*
 * Copyright (c) 2022, Florent Castelli <florent.castelli@gmail.com>
 * Copyright (c) 2022, Sam Atkins <atkinssj@serenityos.org>
 * Copyright (c) 2022, Tobias Christiansen <tobyase@serenityos.org>
 * Copyright (c) 2022, Linus Groh <linusg@serenityos.org>
 * Copyright (c) 2022, Tim Flynn <trflynn89@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Session.h"
#include "Client.h"
#include <LibCore/LocalServer.h>
#include <LibCore/Stream.h>
#include <LibCore/System.h>
#include <unistd.h>

namespace WebDriver {

Session::Session(unsigned session_id, NonnullRefPtr<Client> client, Web::WebDriver::LadybirdOptions options)
    : m_client(move(client))
    , m_options(move(options))
    , m_id(session_id)
{
}

Session::~Session()
{
    if (auto error = stop(); error.is_error())
        warnln("Failed to stop session {}: {}", m_id, error.error());
}

ErrorOr<NonnullRefPtr<Core::LocalServer>> Session::create_server(String const& socket_path, NonnullRefPtr<ServerPromise> promise)
{
    dbgln("Listening for WebDriver connection on {}", socket_path);

    auto server = TRY(Core::LocalServer::try_create());
    server->listen(socket_path);

    server->on_accept = [this, promise](auto client_socket) {
        auto maybe_connection = adopt_nonnull_ref_or_enomem(new (nothrow) WebContentConnection(move(client_socket), m_client, session_id()));
        if (maybe_connection.is_error()) {
            promise->resolve(maybe_connection.release_error());
            return;
        }

        dbgln("WebDriver is connected to WebContent socket");
        m_web_content_connection = maybe_connection.release_value();

        promise->resolve({});
    };

    server->on_accept_error = [promise](auto error) {
        promise->resolve(move(error));
    };

    return server;
}

ErrorOr<void> Session::start()
{
    auto promise = TRY(ServerPromise::try_create());

    auto web_content_socket_path = String::formatted("/tmp/webdriver/session_{}_{}", getpid(), m_id);
    auto web_content_server = TRY(create_server(web_content_socket_path, promise));

    if (m_options.headless) {
        char const* argv[] = {
            "/bin/headless-browser",
            "--webdriver-ipc-path",
            web_content_socket_path.characters(),
            "about:blank",
            nullptr,
        };

        m_browser_pid = TRY(Core::System::posix_spawn("/bin/headless-browser"sv, nullptr, nullptr, const_cast<char**>(argv), environ));
    } else {
        char const* argv[] = {
            "/bin/Browser",
            "--webdriver-content-path",
            web_content_socket_path.characters(),
            nullptr,
        };

        m_browser_pid = TRY(Core::System::posix_spawn("/bin/Browser"sv, nullptr, nullptr, const_cast<char**>(argv), environ));
    }

    // FIXME: Allow this to be more asynchronous. For now, this at least allows us to propagate
    //        errors received while accepting the Browser and WebContent sockets.
    TRY(promise->await());

    m_started = true;
    return {};
}

// https://w3c.github.io/webdriver/#dfn-close-the-session
Web::WebDriver::Response Session::stop()
{
    if (!m_started)
        return JsonValue {};

    // 1. Perform the following substeps based on the remote end’s type:
    // NOTE: We perform the "Remote end is an endpoint node" steps in the WebContent process.
    m_web_content_connection->close_session();

    // 2. Remove the current session from active sessions.
    // NOTE: Handled by WebDriver::Client.

    // 3. Perform any implementation-specific cleanup steps.
    if (m_browser_pid.has_value()) {
        MUST(Core::System::kill(*m_browser_pid, SIGTERM));
        m_browser_pid = {};
    }

    m_started = false;

    // 4. If an error has occurred in any of the steps above, return the error, otherwise return success with data null.
    return JsonValue {};
}

}
