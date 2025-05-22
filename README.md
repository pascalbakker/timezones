A simple timezone program to get time in a given region. Works with daylight savings. Still working on all args impl.

Installation

```
mkdir build && cd $_ && cmake .. && cmake --build .
```

To list timezones at the current time in different regions (can also use -l flag)

```
./Timezones
```

```
Code      TZ                            ISO
UTC       UTC                           2025-03-31T03:21:43
EDT       America/New_York              2025-03-30T23:21:43
PDT       America/Los_Angeles           2025-03-30T20:21:43
BST       Europe/London                 2025-03-31T04:21:43
CEST      Europe/Amsterdam              2025-03-31T05:21:43
IST       Asia/Kolkata                  2025-03-31T08:51:43
CST       Asia/Shanghai                 2025-03-31T11:21:43
AEDT      Australia/Sydney              2025-03-31T14:21:43
JST       Asia/Tokyo                    2025-03-31T12:21:43
-03       America/Sao_Paulo             2025-03-31T00:21:43

```

To pass a custom timezone and time:
```
./Timezones -i "2025-05-22T09:17:26" -t "America/New_York"
```

To pass a custom timezone and custom output timezones

```
./Timezones -i "2025-05-22T09:17:26" -t "America/New_York" -o "Europe/Amsterdam" -o "Europe/London" 
```
