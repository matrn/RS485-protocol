# Changelog


## 17.10.2018
### Added
 * arduino library: broadcast() function, sendUptime() function
 * C code: RS485\_broadcast() function, RS485\_sendUptime() function

## 26.10.2018
 * arduino library and example modifed for usage with HW serial, modified functions RS485protocol() and begin() - see examples for usage

## 27.1.2019
 * fixed memory error, caused by this line `receivedData[recvLen + 1] = '\0';   /* add end char */` - "+ 1" is wrong here
