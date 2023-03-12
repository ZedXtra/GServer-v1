Installation

Zed Server 1.39.22 was built for Linux on the 5.1 kernel with GCC 10.2.1.

If you don't have Linux, you should get it before attempting to use this server.

The hardware requirements for Linux distributions vary. This version was compiled under Debian 11.

The minimum software requirements for this server are:

- MariaDB 15.1
- GCC 10.2.1 (if compiling from source)
- Linux 5.1

The server requires a MariaDB database. The sample database "database.txt" can be used to get you started. This sample database does not include guest accounts. You can define guest accounts in gserver.cpp. Any accounts listed in guestaccounts will be given to users with invalid accounts.

Add to following to /etc/mysql/my.cnf:

[mysqld]
sql_mode = ""