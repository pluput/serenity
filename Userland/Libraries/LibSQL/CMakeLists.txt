set(SOURCES
    AST/CreateSchema.cpp
    AST/CreateTable.cpp
    AST/Describe.cpp
    AST/Expression.cpp
    AST/Insert.cpp
    AST/Lexer.cpp
    AST/Parser.cpp
    AST/Select.cpp
    AST/Statement.cpp
    AST/SyntaxHighlighter.cpp
    AST/Token.cpp
    BTree.cpp
    BTreeIterator.cpp
    Database.cpp
    HashIndex.cpp
    Heap.cpp
    Index.cpp
    Key.cpp
    Meta.cpp
    Result.cpp
    ResultSet.cpp
    Row.cpp
    Serializer.cpp
    TreeNode.cpp
    Tuple.cpp
    Value.cpp
    )

if (SERENITYOS)
    list(APPEND SOURCES SQLClient.cpp)
    set(GENERATED_SOURCES
        ../../Services/SQLServer/SQLClientEndpoint.h
        ../../Services/SQLServer/SQLServerEndpoint.h
    )
endif()

serenity_lib(LibSQL sql)
target_link_libraries(LibSQL PRIVATE LibCore LibIPC LibSyntax LibRegex)
