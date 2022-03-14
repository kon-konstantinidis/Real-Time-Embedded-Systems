# Covid19-Close-Contact-Detection
A C project for the course "Real Time Embedded Systems" of ECE AUTH, implementing an embedded system (purposed to run on a Raspberry Pi Zero) that logs the carriers contacts and covid tests and, in the event of a positive covid test, uploads said contacts in order for them to be notified.
The program is written using pthreads, so the covid tests and close contacts detection (both being events that trigger intermittently at different time intervals) are run parallelly, along with a timer for the program's duration (and end).
