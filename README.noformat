
# deliner

**Cleanly remove text from files** 

## Purpose.
    Debug vs release candidates. 

## Arguments:
    -r                  - recurse 
    -f <file-types>     - File types to check (at least one is required)
    -p <pattern>        - single pattern to run (additional to .deliner file patterns)
    -d <directory>      - directory other than CWD to execute on

## Rules:
    Pattern cannot have more than one *
    Pattern cannot end with *
    Pattern cannot start with *
    First line of files is not subject to patterns.

## conf

.deliner

first line if starts with "#! " is default command if no arguments are provided
Each line not starting with "#" is a pattern to remove

i.e

""""""""""""""""""""""""
#! -r -f cpp,hpp 

<PATTERN>
KLOG(INF)*;
""""""""""""""""""""""""


