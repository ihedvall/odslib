# ODS C++ Library Repository

## Summary

The ODS C++ Library implements an interface against ASAM ODS databases. The repository include a library that unifies the 
interface to an ASAM ODS database. The repository also includes applications that are based upon the above C++
library. The library and its applications should work on most operating system but only windows is regular maintained
due to lack of resources. Please contact me if you want support on macOS and Linux.

The repository is still in beta mode but is fairly stable for end-users. An installation kit can be done on request. 

### ODS C++ Library
Library that simplifies reading and writing from/to an ODS database. Currently, are the SQLite and Postgres databases 
supported. Support for other database is done on request.

### ODS Configurator 
Configuration GUI tool for building, creating and maintaining ODS databases. The application is built with the 
wxWidgets framework. Display of table data is not supported as it exists a lot of external applications that does that. 

### Event Log gRPC Server
The library do implement a syslog server database. To access this database, a gRPC server interface was created. 
Note that the actual Event Log Server is implemented based upon a workflow engine and is implemented in another 
repository [Event Log](https://github.com/ihedvall/eventlog). 

### Report Explorer (Deprecated)
The is an application that search for new measurement file in a directory and generate reports. This application is
not intended for new installations. It will be replaced by a workflow base design so the end-user can modify the 
report server functionality.

## Building the project
The project uses CMAKE for building but requires a lot of third-party libraries. The following third-party libraries 
are used and needs to be downloaded/built.

- Boost Library. 
- Expat Library. 
- OpenSSL Library. 
- GRPC/Protobuf Library. The gRPC includes the protobuf library.
- ZLIB Library.
- SQLite3 Library.
- LibPQ (Postgres) Library.
- wxWidgets Framework. Used by the configuration tool.
- Doxygen's application. Is required if the documentation should be built.
- Google Test Library. Is required for running and build the unit tests.

## License

The project uses the MIT license. See external LICENSE file in project root.

