/*
* Copyright 2008, 2009 Free Software Foundation, Inc.
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




#include "Transceiver.h"
#include "GSMCommon.h"
#include <time.h>

#include <Logger.h>
#include <Configuration.h>

ConfigurationTable gConfig;

using namespace std;

int main(int argc, char *argv[]) {

  // Configure logger.
  if (argc<2) {
    cerr << argv[0] << " <logLevel> [logFilePath]" << endl;
    cerr << "Log levels are ERROR, ALARM, WARN, NOTICE, INFO, DEBUG, DEEPDEBUG" << endl;
    exit(0);
  }
  gLogInit(argv[1]);
  if (argc>2) gSetLogFile(argv[2]);

  srandom(time(NULL));

  Device *usrp = Device::make(400.0e3);
  if (!usrp->open()) {
    cerr << "Device open failed. Exiting..." << endl;
    exit(1);
  }

  RadioInterface* radio = new RadioInterface(usrp,3);
  Transceiver *trx = new Transceiver(5700,"127.0.0.1",SAMPSPERSYM,GSM::Time(3,0),radio);
  trx->transmitFIFO(radio->transmitFIFO());
  trx->receiveFIFO(radio->receiveFIFO());

  trx->start();
  //int i = 0;
  while(1) { sleep(1); }//i++; if (i==60) break;}
}
