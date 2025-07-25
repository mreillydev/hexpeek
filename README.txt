                          hexpeek version 1.1.20250722
                          ----------------------------

Documentation available by running 'hexpeek -help'.

Thank you for trying hexpeek. Send your comments to hexpeek@hexpeek.com.


                           Version 1.1 Release Notes
                           -------------------------

1. Allow singleton zero "0" as data input for operations.
2. Create "len" as alias keyword for "-0" (known file length).
3. Refuse to execute "diff" by default in interactive mode.
4. Refuse to execute replace and insert commands to "max" by default.
5. Refuse partial recovery if only one backup file is present by default.
6. Fallback to opening data infiles read-only if backup files can't be opened.
7. Release advanced test suite and doxygenable documentation.
8. Add test target to run tests from top level Makefile.


                                  Dependencies
                                  ------------

Program:
                C99 compiler
                C POSIX.1-2008 standard library
    [optional]  libedit for editable console support (www.thrysoee.dk/editline)

Tests:
                POSIX sh shell


                                    License
                                    -------

Any file contained in this distribution which lacks its own copyright notice
is licensed according the file LICENSE in this directory.


                                   Trademark
                                   ---------

hexpeek is a trademark of Michael Reilly.

Copyright-compliant redistributions of the open source release of hexpeek are
permitted and encouraged to use "hexpeek" as a package, directory, file, and
documentation name provided that the above trademark notice is preserved, the
redistribution is not identified as being derived from any release of hexpeek
other than the open source one, and modified redistributions are clearly marked
as such. The hexpeek mark must not be used in a manner which confuses the
ownership of the mark.


                              Contact Information
                              -------------------

For more information, please visit https://www.hexpeek.com.

hexpeek is now also on github: https://github.com/mreillydev/hexpeek

Or send e-mail to hexpeek@hexpeek.com. A public key can be found in etc/.
