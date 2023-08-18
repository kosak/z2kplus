# Directory structure

At startup the system is told about four root directories:
* logged (aka permanent plaintext): all permanent (non-graffiti) messages
* unlogged (aka graffiti plaintext): "graffiti" messages (expire/vanish after 7 days)
* index: derived information. Currently this is the single file z2k.index
* scratch: Scratch files used for building z2k.index.
* media: A directory where uploaded media (like pasted images) is stored and served from.

The first two (logged and unlogged) represent "ground truth" information that is not recoverable in
the event of a catastrophe. Of these, "logged" is precious and should be backed up. As a policy
matter, "graffiti" should not be backed up, because people expect that the messages permanently
disappear after 7 days. In the even that the system fails catastrophically, there is the risk
that all graffiti messages could be lost after recovery. This risk is acceptable, as they would
have only been valid for at most 7 more days anyway.

The third one (index) contains one file, z2k.index file, which contains the custom database /
reverse index that the code has built. The system maintains this, and it is rebuilt from scratch
on every startup and on every periodic index rebuild. It does not need to be (and should not be)
backed up.

## Substructure of the "logged" root

~~~
yyyy/
  mm/
    yyyymmdd.logged.NNN
~~~

This is one of NNN plaintext files (starting with 000) for yyyymmdd.  Initially there is one
plaintext file (namely 000) created for a given day. There is an additional file created every time
the sytem is restarted or there is an index rebuild. As this doesn't happen very often, NNN is in
practice very small. If some bizarre edge case should happen where there are 1000 files in a given
day, the system will fail to start. In this case, the admin should manually fix the data by, e.g.,
concatenating all the files into a single 000 file.

The purpose of this is to allow concurrent index rebuilds. When it is time to reindex, the running
system can start a new log file, say, yyyymmdd.004.logged. Meanwhile the reindexer can safely
process all files up to but not including yyyymmdd.004.logged, since those files will never change
again.

The purpose of the yyyy and mm directories is so that any particular directory doesn't get loaded
up with many thousands of files.

## Substructure of the unlogged root

This is the same as the logged root except the filename contains the word "unlogged"
rather than "logged".

## Substructure of the index root.

~~~
z2k.index # the custom database / reverse index
~~~

## Substructure of the scratch root

~~~
(variety of temp files with arbitrary names) - used to build the reverse index. Currently these
are left around for debugging purposes (until the next rebuild) but they could also be deleted
right after rebuild.
~~~

## Index rebuild process

When starting up, and periodically while the system is running, we do an index rebuild. This is a
sketch of the steps.

1. If the system is running, close the current log files (logged and unlogged), open new ones,
   and note the keys of the closed files. These keys will be the last log files to be indexed.
   If you like half-open intervals (and I do), the keys of the newly opened permanent and graffiti
   files will be the "end" keys for the permanent and graffiti logs, respectively.
   These keys are kept in sync, so they will be the same yyyymmdd.NNN for both logged and unlogged.
2. Calculate the "unlogged start date" (basically now minus one week) and use this to determine
   the unlogged begin key. (Unlogged files before this key will not be indexed, because in fact
   they will be deleted shortly).
3. Empty the scratch directory
4. Build the index in "scratch", based on:
- permanent files from the beginning of time through the last logged key (identified in step 1).
- graffiti files between the first unlogged key (identified in step 2) and the last unlogged key
  (identified in step 1). This takes a little while, maybe a minute or so.
5. Once the new z2k.index is ready, freeze the interactive system (stop the world) and fix
   up the files:
   - Delete z2k.index from index
   - Move z2k.index from scratch to index
   - (optional) clear out the rest of the scratch directory
6. Delete graffiti files prior to the unlogged begin key
7. Unfreeze the interactive system, telling it everything has changed. It will need to load
   the new indices in current, and it will need to read "recent" logged and unlogged messages
   starting from the "begin" keys into its RAM. It will also need to fix up various aspects
   of its state that may have pointed to the old index (hopefully this is not much).

# WordIndexes

Messages are parsed into words, and the resulting stream of words is (conceptually at least)
stored in the database for later searching. Each word in a message is given a globally
unique index, from zero and monotonically increasing. For example, consider the following two
messages:

| sender   | signature       | instance      | message                   |
|----------|-----------------|---------------|---------------------------|
| kosak    | Corey Kosak     | chat.pie      | I (like) pie.             |
| kosh     | Kosh            | chat.pie.not  | Your pie is not ready.    |

The assigned wordIndexes would be the indexes of the tokens of the above, in order. "tokens"
are basically either sequences of (very-permissive-alphanumeric), or a single ASCII punctuation
character, but with the exception that an inner apostrophe doesn't break a word, so "isn't"
is a single token. "very-permissive-alphanumeric" is basically A-Za-z0-9 plus any non-ASCII
character in Unicode. The above messages would expand as:


| wordIndex | token    |
|-----------|----------|
| 0         | kosak    |
| 1         | Corey    |
| 2         | Kosak    |
| 3         | chat     |
| 4         | .        |
| 5         | pie      |
| 6         | I        |
| 7         | (        |
| 8         | like     |
| 9         | )        |
| 10        | pie      |
| 11        | .        |
| 12        | kosh     |
| 13        | Kosh     |
| 14        | chat     |
| 15        | .        |
| 16        | pie      |
| 17        | .        |
| 18        | not      |
| 19        | Your     |
| 20        | pie      |
| 21        | is       |
| 22        | not      |
| 23        | ready    |
| 24        | .        |

...and so on. Note this is simply a way of breaking the mesage stream into words and locating those
words. It does not (at this point) have anything to do with compression, duplicate words, etc.
That is the job of the reverse index. Conceptually the reverse index orders groups and orders
the above table by token and orders each group by wordIndex:

| wordIndex     | token    |
|---------------|----------|
| 4,11,15,17,24 | .        |
| 7             | (        |
| 9             | )        |
| 1             | Corey    |
| 6             | I        |
| 2             | Kosak    |
| 13            | Kosh     |
| 19            | Your     |
| 3,14          | chat     |
| 21            | is       |
| 0             | kosak    |
| 12            | kosh     |
| 8             | like     |
| 18,22         | not      |
| 5,10,16,20    | pie      |
| 23            | ready    |

The tokens are stored as a trie whose values are arrays of wordIndices hanging off the nodes of
that trie.

There is also another simple data structure (an array) mapping wordIndex to messageIndex.
For the above example this is simply

| wordIndex | messageIndex  |
|-----------|---------------|
| 0         | 0             |
| 1         | 0             |
| 2         | 0             |
| 3         | 0             |
| 4         | 0             |
| 5         | 0             |
| 6         | 0             |
| 7         | 0             |
| 8         | 0             |
| 9         | 0             |
| 10        | 0             |
| 11        | 0             |
| 12        | 1             |
| 13        | 1             |
| 14        | 1             |
| 15        | 1             |
| 16        | 1             |
| 17        | 1             |
| 18        | 1             |
| 19        | 1             |
| 20        | 1             |
| 21        | 1             |
| 22        | 1             |
| 23        | 1             |
| 24        | 1             |

This could also have been done as a more dense table using starting word indices and binary search,
but for search speed and implementation laziness we have not done this.
