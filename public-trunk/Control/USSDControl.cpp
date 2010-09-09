/**@file USSD Control (L3) */
/*
* Copyright 2008, 2009, 2010 Free Software Foundation, Inc.
*
* This software is distributed under the terms of the GNU Affero Public License.
* See the COPYING file in the main directory for details.
*
* This use of this software may be subject to additional restrictions.
* See the LEGAL file in the main directory for details.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/


#include <stdio.h>
#include <GSMLogicalChannel.h>
#include <GSML3MMMessages.h>
#include "ControlCommon.h"
#include <Regexp.h>
#include "GSML3NonCallSSMessages.h"
#include "GSML3NonCallSSComponents.h"
#include "GSML3NonCallSSElements.h"
#include <algorithm>


using namespace std;
using namespace GSM;
using namespace Control;


#include "SIPInterface.h"
#include "SIPUtility.h"
#include "SIPMessage.h"
#include "SIPEngine.h"
using namespace SIP;

/** @file USSDControl.cpp

USSD string coding schema is described at GSM 02.90 5.1.2, 5.1.4
and GSM 02.30 4.5

TODO:
1) We should support Network -> MS USSD too.

BUGS:
1) *101# does not work with Siemens A52 (but does work with A65).
After phone receives "Send more data" it can't perform any more USSD
and can't reply to this request too. If you try, phone displays
"NOT EXECUTED". BUT when you switch it off afterwards, it sends
ReleaseComplete, so it seems it's stuck waiting for something from
network?

2) On Alcatel BG3 message sent with Notify disappears as soon as
phone receives our ReleaseComplete. Probably we should delay it
by a few seconds?
*/


L3Frame* getFrameUSSD (LogicalChannel *LCH, GSM::Primitive primitive=DATA)
{
	L3Frame *retVal = LCH->recv(gConfig.getNum("USSD.timeout"),0);
	if (!retVal) {
		LOG(NOTICE) <<"TIME";
		//throw ChannelReadTimeout();
		retVal = NULL;
	}
	else if (retVal->primitive() != primitive) {
		LOG(NOTICE) << "unexpected primitive, expecting " << primitive << ", got " << *retVal;
		//throw UnexpectedPrimitive();
		retVal = NULL;
	}
	else if ((retVal->primitive() == DATA) && (retVal->PD() != L3NonCallSSPD)) {
		LOG(NOTICE) << "unexpected (non-USSD) protocol in frame " << *retVal;
		//throw UnexpectedMessage(0, retVal);
		retVal = NULL;
	}
	return retVal;
}

void USSDSend(string USSDString, unsigned InvokeID, unsigned TI, unsigned TIFlag, LogicalChannel* LCH, Control::USSDData::USSDMessageType messageType)
{
	L3Frame Frame;
	
	if (TIFlag) TIFlag = 0;
	else TIFlag = 1;

	if (messageType == Control::USSDData::REGrequest)
	{
		//SEND MT REGISTER, UnstructuredSSRequest
		L3NonCallSSComponentInvoke* ComponentPtr = new L3NonCallSSComponentInvoke(L3NonCallSSInvokeID(InvokeID), 
							L3NonCallSSOperationCode(GSM::L3NonCallSSOperationCode::UnstructuredSSRequest));
		ComponentPtr->parameters(L3NonCallSSParameters(USSDString.c_str()));
		L3NonCallSSRegisterMessage RegisterMessage(0, 0, ComponentPtr);
		RegisterMessage.write(Frame);	
		LOG(DEBUG) << "Sending Register Message: " << RegisterMessage;
		LCH->send(Frame);
	}
	
	else if (messageType == Control::USSDData::request)
	{
		//SEND MT Facility, UnstructuredSSRequest
		L3NonCallSSComponentInvoke* ComponentPtr = new L3NonCallSSComponentInvoke(L3NonCallSSInvokeID(InvokeID), 
						L3NonCallSSOperationCode(GSM::L3NonCallSSOperationCode::UnstructuredSSRequest));
		ComponentPtr->parameters(L3NonCallSSParameters(USSDString.c_str()));
		L3NonCallSSFacilityMessage FacilityMessage(TIFlag, TI, ComponentPtr);
		FacilityMessage.write(Frame);	
		LOG(DEBUG) << "Sending Register Message: " << FacilityMessage;
		LCH->send(Frame);
	}

	else if (messageType == Control::USSDData::response)
	{
		//send response
		L3NonCallSSComponentReturnResult* ComponentPtr = new L3NonCallSSComponentReturnResult(L3NonCallSSInvokeID(InvokeID));
		ComponentPtr->operationCode(L3NonCallSSOperationCode(GSM::L3NonCallSSOperationCode::ProcessUnstructuredSSRequest));
		ComponentPtr->parameters(L3NonCallSSParameters(USSDString.c_str()));
		L3NonCallSSReleaseCompleteMessage ReleaseCompleteMessage(TIFlag, TI);
		ReleaseCompleteMessage.component(ComponentPtr);
		ReleaseCompleteMessage.write(Frame);
		LOG(DEBUG) << "Sending Release Complete Message with response: " << ReleaseCompleteMessage;
		LCH->send(Frame);
		//send L3 Channel Release 
		LOG(DEBUG) << "L3 Channel Release.";
		LCH->send(L3ChannelRelease());
	}

	else if (messageType == Control::USSDData::request )
	{
		//send request
		if(InvokeID == 0xff) InvokeID = 0;
		else InvokeID++;
		L3NonCallSSComponentInvoke* ComponentPtr = new L3NonCallSSComponentInvoke(L3NonCallSSInvokeID(InvokeID), 
								L3NonCallSSOperationCode(GSM::L3NonCallSSOperationCode::UnstructuredSSRequest));

		ComponentPtr->parameters(L3NonCallSSParameters(USSDString.c_str()));
		L3NonCallSSFacilityMessage FacilityMessage(TIFlag, TI, ComponentPtr);
		FacilityMessage.write(Frame);	
		LOG(DEBUG) << "Sending Facility Message with request: " << FacilityMessage;
		LCH->send(Frame);
	}

	else if (messageType == Control::USSDData::notify)
	{
		//send notify
		if(InvokeID == 0xff) InvokeID = 0;
		else InvokeID++;
		L3NonCallSSComponentInvoke* ComponentPtr = new L3NonCallSSComponentInvoke(L3NonCallSSInvokeID(InvokeID),
								L3NonCallSSOperationCode(GSM::L3NonCallSSOperationCode::UnstructuredSSNotify));
		ComponentPtr->parameters(L3NonCallSSParameters(USSDString.c_str()));
		L3NonCallSSFacilityMessage FacilityMessage(TIFlag, TI, ComponentPtr);
		FacilityMessage.write(Frame);
		LOG(DEBUG) << "Sending Facility Message with notify: " << FacilityMessage;
		LCH->send(Frame);
	}

	else if (messageType == Control::USSDData::error) 
	{
		//send error
		L3NonCallSSComponentReturnError* ComponentPtr = new L3NonCallSSComponentReturnError(L3NonCallSSInvokeID(InvokeID),
								L3NonCallSSErrorCode(GSM::L3NonCallSSErrorCode::SystemFailure));
		L3NonCallSSFacilityMessage FacilityMessage(TIFlag, TI, ComponentPtr);
		FacilityMessage.write(Frame);
		LOG(DEBUG) << "Sending Facility Message with error System Failure: " << FacilityMessage;
		LCH->send(Frame);
	}

	else if(messageType == Control::USSDData::release)
	{
		// NOTE: On some phones (e.g. Nokia 3310, Alcatel BG3) if this parameter is set to a non-empty string,
		//       when it is sent after Notify USSD, this string is shown on the screen after showing Notify
		//       message just for a second. I.e. it replaces (obscures) original Notify message. Thus we
		//       recommend to set this string to an empty string.
		L3NonCallSSComponentReturnResult* ComponentPtr = new L3NonCallSSComponentReturnResult(L3NonCallSSInvokeID(InvokeID)); 
		ComponentPtr->operationCode(L3NonCallSSOperationCode(GSM::L3NonCallSSOperationCode::ProcessUnstructuredSSRequest));
		ComponentPtr->parameters(L3NonCallSSParameters("release"));
		L3NonCallSSReleaseCompleteMessage ReleaseCompleteMessage(TIFlag, TI);
		if (TIFlag == 1)
		{
			ReleaseCompleteMessage.component(ComponentPtr);
		}
		ReleaseCompleteMessage.write(Frame);
		LOG(DEBUG) << "Sending Release Complete Message: " << ReleaseCompleteMessage;
		LCH->send(Frame);
		//send L3 Channel Release 
		LOG(DEBUG) << "L3 Channel Release.";
		LCH->send(L3ChannelRelease());
	}



}

Control::USSDData::USSDMessageType USSDParse(L3NonCallSSMessage* Message, string* USSDString, unsigned* InvokeID )
{
	L3Frame Frame;
	Control::USSDData::USSDMessageType messageType = Control::USSDData::error;
	// Low-level message flow is described at GSM 04.90 6.1.
	// High-level message flow is described at GSM 03.90 6.2.

	// Process REQUEST message
	if (Message->MTI() == GSM::L3NonCallSSMessage::Register)
	{
		LOG(DEBUG) << "Received Register Message";
		L3NonCallSSRegisterMessage* RegisterMessage = (L3NonCallSSRegisterMessage*)Message;
		if (RegisterMessage->l3NonCallSSComponent()->NonCallSSComponentTypeTag()== GSM::L3NonCallSSComponent::Invoke)
		{
			LOG(DEBUG) << "Received Invoke Component";
			L3NonCallSSComponentInvoke* Component;
			Component = (L3NonCallSSComponentInvoke*)RegisterMessage->l3NonCallSSComponent();
			*USSDString = Component->l3NonCallSSParameters().data();
			LOG(DEBUG) << "Read USSD string";
			*InvokeID = (unsigned)Component->invokeID().value();
			messageType = Control::USSDData::request;
		}
		
	}
	// Process RESPONSE message
	else if (Message->MTI() == GSM::L3NonCallSSMessage::Facility)
	{
		LOG(DEBUG) << "Received Facility Message";
		L3NonCallSSFacilityMessage* FacilityMessage = (L3NonCallSSFacilityMessage*)Message;
		if (FacilityMessage->l3NonCallSSComponent()->NonCallSSComponentTypeTag()== GSM::L3NonCallSSComponent::ReturnResult)
		{
			LOG(DEBUG) << "Return Result Component";
			L3NonCallSSComponentReturnResult* Component;
			Component = (L3NonCallSSComponentReturnResult*)FacilityMessage->l3NonCallSSComponent();
			if (Component->haveL3NonCallSSParameters()) *USSDString = Component->l3NonCallSSParameters().data();
			else *USSDString = "";
			*InvokeID = (unsigned)Component->invokeID().value();
			messageType = Control::USSDData::response;
		}
	}

	// Process RELEASE or ERROR
	else // MTI == GSM::L3NonCallSSMessage::ReleaseComplete
	{
		L3NonCallSSReleaseCompleteMessage* ReleaseCompleteMessage = (L3NonCallSSReleaseCompleteMessage*)Message;
		if (ReleaseCompleteMessage->haveL3NonCallSSComponent())
		{
			*InvokeID = ReleaseCompleteMessage->l3NonCallSSComponent()->invokeID().value();
			messageType = Control::USSDData::release;
		}
		else
		{
			InvokeID = 0;
			messageType = Control::USSDData::release;
		}
		*USSDString = "";
	}

	return messageType;
}

void Control::MOUSSDController(const L3CMServiceRequest *request, LogicalChannel *LCH)
{
	assert(request);
	assert(LCH);
	
	LOG(DEBUG) << "Get L3 CM Service Request: " << *request;
	L3MobileIdentity mobileIdentity = request->mobileIdentity();
	LOG(DEBUG) << "mobileIdentity: "<<mobileIdentity;
	LOG(DEBUG) << "Send L3 CM Service Accept";
	LCH->send(L3CMServiceAccept());

	//Get USSD frame from MS
	LOG(DEBUG) << "Get USSD frame from MS";
	L3Frame *USSDFrame = getFrameUSSD(LCH);
	//Parse USSD frame
	LOG(DEEPDEBUG) << "Parse USSD frame";
	L3NonCallSSMessage* USSDMessage;
	USSDMessage = parseL3NonCallSS(*USSDFrame);
	LOG(INFO) << "USSD message:"<<*USSDMessage;
	
	string USSDString = "";
	unsigned InvokeID = 0;

	Control::USSDData::USSDMessageType messageType = USSDParse(USSDMessage, &USSDString, &InvokeID);
	unsigned transactionID = USSDDispatcher (mobileIdentity, USSDMessage->TIFlag(), USSDMessage->TIValue(), messageType, USSDString, true);
	unsigned TI = USSDMessage->TIValue();
	
	TransactionEntry transaction;
	while(gTransactionTable.find(transactionID, transaction))
	{
		if (transaction.Q931State() == Control::TransactionEntry::USSDclosing)
		{
			break;
		}
		else if (transaction.ussdData()->waitNW()==0)
   		{
			gTransactionTable.find(transactionID, transaction);
			//SEND
			USSDSend(transaction.ussdData()->USSDString(), InvokeID, TI, 0, LCH, transaction.ussdData()->MessageType());
			if((transaction.ussdData()->MessageType() == Control::USSDData::response)||
				(transaction.ussdData()->MessageType() == Control::USSDData::release))
			{ 
				LOG(DEBUG) << "USSD waitMS response||relese";
				transaction.Q931State(Control::TransactionEntry::USSDclosing);
			}
			else
			{		
				//RECV
				L3Frame *USSDFrame = getFrameUSSD(LCH);
				if (USSDFrame == NULL) 
				{
					USSDSend(transaction.ussdData()->USSDString(), InvokeID, TI, 0, LCH, Control::USSDData::release);
					transaction.Q931State(Control::TransactionEntry::USSDclosing);
				}
				else
				{
					//Parse USSD frame
					L3NonCallSSMessage* USSDMessage = parseL3NonCallSS(*USSDFrame);
					TI = USSDMessage->TIValue();
					LOG(INFO) << "USSD message before PARSE:"<<*USSDMessage;
					Control::USSDData::USSDMessageType messageType = USSDParse(USSDMessage, &USSDString, &InvokeID);
					LOG(INFO) << "MO USSD message: "<< *USSDMessage << " MO USSD type: "<< messageType
					          << " USSD string: " << USSDString << " InvokeID: " << InvokeID;
					transaction.ussdData()->USSDString(USSDString);
					transaction.ussdData()->MessageType(messageType);
					delete USSDMessage;
				}
				delete USSDFrame;
								
			}
			gTransactionTable.update(transaction);
			transaction.ussdData()->postMS();
		}
	}
}


void Control::MTUSSDController(TransactionEntry& transaction, LogicalChannel *LCH)
{

	assert(LCH);
	LOG(INFO) << "MTUSSD: transaction: "<< transaction;
	LCH->transactionID(transaction.ID());	
	string USSDString = "";
	unsigned InvokeID = 0;
	unsigned TI = 0;
	unsigned transactionID = transaction.ID();

	transaction.Q931State(Control::TransactionEntry::USSDworking);
	gTransactionTable.update(transaction);

	while(gTransactionTable.find(transactionID, transaction))
	{
		if (transaction.Q931State() == Control::TransactionEntry::USSDclosing)
		{
			break;
		}
		else if (transaction.ussdData()->waitNW()==0)
   		{
			gTransactionTable.find(transactionID, transaction);
			//SEND
			USSDSend(transaction.ussdData()->USSDString(), InvokeID, TI, 1, LCH, transaction.ussdData()->MessageType());
			if((transaction.ussdData()->MessageType() == Control::USSDData::response)||
				(transaction.ussdData()->MessageType() == Control::USSDData::release))
			{ 
				LOG(DEBUG) << "USSD waitMS response||relese";
				transaction.Q931State(Control::TransactionEntry::USSDclosing);
			}
			else
			{		
				//RECV
				L3Frame *USSDFrame = getFrameUSSD(LCH);
				if (USSDFrame == NULL) 
				{
					USSDSend(transaction.ussdData()->USSDString(), InvokeID, TI, 1, LCH, Control::USSDData::release);
					transaction.Q931State(Control::TransactionEntry::USSDclosing);
				}
				else
				{
					//Parse USSD frame
					L3NonCallSSMessage* USSDMessage = parseL3NonCallSS(*USSDFrame);
					TI = USSDMessage->TIValue();
					Control::USSDData::USSDMessageType messageType = USSDParse(USSDMessage, &USSDString, &InvokeID);
					LOG(INFO) << "MT USSD message: "<< *USSDMessage << "MT USSD type: "<< messageType
					          << " USSD string: " << USSDString << " InvokeID: " << InvokeID;
					transaction.ussdData()->MessageType(messageType);
					transaction.ussdData()->USSDString(USSDString);
					delete USSDMessage;
				}
				delete USSDFrame;				
			}
			gTransactionTable.update(transaction);
			transaction.ussdData()->postMS();
		}
	}

	LOG(DEBUG) << "USSD session done.";
}


