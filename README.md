# Library ODS

## Summary

The ODS Library implements an interface against an ASAM ODS database. The purpose is that threat all databases 
as an ODS database. The library also include interfaces to various databases.

The library is still in beta mode meaning that major changes may occur. Only SQLite databases are currently 
implemented.

## Building the project

The project uses CMAKE for building. The following third-party libraries are used and
needs to be downloaded and built.

- Boost Library. Set the 'Boost_ROOT' variable to the Boost root path.
- Expat Library. Set the 'EXPAT_ROOT' variable to the expat root path.
- OpenSSL Library. Set the 'OPENSSL_ROOT' variable to the OpenSSL root path.
- ZLIB Library. Set the 'ZLIB_ROOT' variable to the ZLIB root path.
- Doxygen's application. Is required if the documentation should be built.
- Google Test Library. Is required for running and build the unit tests.

## License

The project uses the MIT license. See external LICENSE file in project root.

