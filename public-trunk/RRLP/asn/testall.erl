-module(testall).
-export([run/0]).

%%% dump out contents of all files in ../tests using asn1 generated code
run() ->
    make:all(),
    {ok,Files}=file:list_dir("../tests"),
    io:format("Decoding: ~s~n~n", [string_join(Files, ",")]),
    lists:foreach(fun(X) -> runtest(X) end, [X||X<-Files,ends_with_per(string:tokens(X,"."))]).

runtest(Filename) ->
    {ok,Contents}=file:read_file("../tests/" ++ Filename),
    io:format("~s: ~w~n~w~n", [Filename, Contents, rrlp:decode(Contents)]).

ends_with_per([_,"per"]) ->
    true;
ends_with_per(_) ->
    false.

%% Some util
string_join(Items, Sep) ->
    lists:flatten(lists:reverse(string_join1(Items, Sep, []))).

string_join1([Head | []], _Sep, Acc) ->
    [Head | Acc];
string_join1([Head | Tail], Sep, Acc) ->
    string_join1(Tail, Sep, [Sep, Head | Acc]).

