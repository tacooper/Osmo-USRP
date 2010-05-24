-module(rrlp).
-export([decode/1, decode_pos/1, decodePDU/1, encodePDU/1]).

%%% Module to decode RRLP messages. All the heavy work is done in RRLP.erl which is
%%% automatically generated from asn (see Makefile).
%%% The only decoding going on by hand here is that of the Position returned, following 3GPP TS 23.032.

decode(Data) ->
    case 'RRLP':decode('PDU', Data) of
        {error, Bla} -> {error, Bla};
        {ok, {'PDU', _referenceNumber, Component}} ->
            decodeComponent(Component)
    end.

decodeComponent(Component) ->
    case Component of
        {msrPositionRsp,MsrPositionRsp} -> decodeMsrPositionRsp(MsrPositionRsp);
        {assistanceDataAck,'NULL'} -> {assitanceDataAck};
        {msrPositionReq,{'MsrPosition-Req', PositionReq,_,_,_,_,_,_,_,_}} -> {msrPositionReq, PositionReq}
        %_ -> Component % catch all for debugging - remove!
    end.

decodeMsrPositionRsp({'MsrPosition-Rsp',_,_,_,{'LocationInfo',_Reference,_TOW,_FixType,PositionInfo},
     _,_,_,_,_,_}) -> decode_pos(list_to_binary(PositionInfo));
decodeMsrPositionRsp({'MsrPosition-Rsp',_,_,_,_,_,{'LocationError',gpsAssDataMissing,{'AdditionalAssistanceData',AdditionalAssistanceData,_,_}},_,_,_,_}) ->
        {additionalAssistanceData, AdditionalAssistanceData}.


% Decode the Position part from an RRLP Position Response.
% 7.3.5 Ellipsoid Point
decode_pos(<<8:4,_Spare:4,LatSign:1,Lat23:23,Lon24:24,AltDir:1,Alt15:15,_Rest/binary>>) ->
    io:format("Parsing 8~n", []),
    {(1 - LatSign*2)*Lat23*90/(1 bsl 23), Lon24*360/(1 bsl 24), (1-AltDir*2)*Alt15 } ;
% 7.3.6 Ellipsoid Point with altitude and uncertainty ellipsoid - we ignore the uncertainty
decode_pos(<<9:4,_Spare:4,LatSign:1,Lat23:23,Lon24:24,AltDir:1,Alt15:15,Rest/binary>>) ->
    io:format("Parsing 9 - Lat23 ~s Rest ~s~n", [util:bin_to_zeroones(<<Lat23:23>>),
        util:bin_to_zeroones(<<Rest/binary>>)]),
    {(1 - LatSign*2)*Lat23*90/(1 bsl 23), Lon24*360/(1 bsl 24), (1-AltDir*2)*Alt15 };
% 7.3.1 Ellipsoid Point or anything else really (catch all)
decode_pos(<<What:4,_Spare:4,LatSign:1,Lat23:23,Lon24:24,_Rest/binary>>) ->
    io:format("Parsing ~w~n", [What]),
    {(1 - LatSign*2)*Lat23*90/(1 bsl 23), Lon24*360/(1 bsl 24), 0.0}.

% Raw Encoding/Decoding
encodePDU(Data) ->
    'RRLP':encode('PDU', Data).

decodePDU(Data) ->
    'RRLP':decode('PDU', Data).

