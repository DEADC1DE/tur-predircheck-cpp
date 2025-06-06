
--[ Intro ]----------------------------------------------------------

Tur-Pedircheck rewritten in C++

Tur-PreDirCheck. A script that runs before users create a dir to make
sure the directory follows a set of rules.

--[ Installation ]---------------------------------------------------

Compile predircheck.cpp

g++ -std=c++17 -Wall -o tur-predircheck tur-predircheck.cpp

Copy tur-predircheck to /glftpd/bin

Make it executable by chmod 755 /glftpd/bin/tur-predircheck

Load it in glftpd.conf as: 
 pre_dir_check /bin/tur-predircheck

Remove any other possible pre_dir_check in the config.
You might want to wait with this until you configured it =)

-> Make sure you have the following binaries in /glftpd/bin:
-> tr, egrep, grep, cut & dirloglist_gl (read on)

Very few checks are done to see if you configured it right is done to speed
it up so make damn sure you have the bins and configure it right =)



Now, this script has 6 functions.
1: To prevent users from creating, say, CD1 when Cd1 already
   exists.

2: To prevent users from creating a directory that was/is
   on the site in another section or was nuked.

3: To prevent users from creating a dir from a banned group.

4: To prevent users from creating a dir which matches something else
   like NUKED-.

5: To only allow certain dirs in certain places.

6: To block creating a directory which already exists in the parent.
   Like /site/Testing/testing will be blocked as testing already exists.

Nr1 is always in use. Nr2, Nr3, Nr4, Nr5 & Nr6 can be turned off though.

If you plan on using Nr2, do this as well:

 (note, if you only want to check if the release was previously nuked in the
  same directory, you can use NUKE_PREFIX= instead of this.)

 Included in this package is dirloglist_gl. Both the binary and the source.
 In the glftpd1 directory is the one for glftpd 1.* and with that info, you
 can guess whats in the glftpd2 directory. There is also a glftpd2.12 which is
 probably what you want in this modern world =)

 It was compiled on RedHat9 if you want to use the binary. In other case,
 configure the source as: gcc dirloglist_gl.c -o /glftpd/bin/dirloglist_gl

 (glftpd2.12 version was compiled on Fedora 34) 

 Its simply modified to work inside glftpd instead of just shell as with
 the original dirloglist.
 The following line:
  sprintf(dirlogfile, "%s/logs/dirlog", datapath);
 Is replaced by:
  sprintf(dirlogfile, "/ftp-data/logs/dirlog", datapath);

 Run a chmod 755 /glftpd/bin/dirloglist_gl as well.

 Test your original dirloglist by running it from shell. It should spit out
 a lot of lines containing your directories.
 To test dirloglist_gl, do:
  chroot /glftpd
  cd bin
  ./dirloglist_gl
 and it should spit out the same thing.


Now then, on to the settings in tur-predircheck

--[ Settings ]-------------------------------------------------------

SKIPSECTIONS  = A number of paths or part of paths where the script will just quit
                in. It will match anything in the $PWD so GROUPS would skip any path
                or release that includes GROUPS. Therefore, use /GROUPS to make it
                a bit more secure. Add as many as you like, space seperated.
                Specifying a tailing slash is not recomended ( no /SVCD/ ).

                As you can see, you can specify part of the path or the full path.                

SKIPUSERS     = A number of users you wish to exclude. Specify like "^user1$|^user2$"
                See SKIPDIRS below for explanation on ^ and $.

                Why not just make it space seperated? Because then I would have to modify
                SKUPUSERS in the script so I can use egrep on it and, which this works, 
                would take extra time. Something we dont have when it comes to creating a
                dir. So you'll just have to deal with the format.

SKIPGROUPS    = Same as above, but for groups. "^group1$|^group2$|^etc$"

                This only works on the users PRIMARY group and not any secondary ones.
                Why? Because as with SKIPUSERS, this should be fast to execute. If I wanted
                to check all the groups on the user I would have to read the userfile and do
                a loop for all "GROUP". The primary group, however, is always stored in the
                variable $GROUP, which is what I check instead. Much faster.

SKIPFLAGS     = And once again, exactly like above but for flags. Dont use ^ and $ here
                though. Just "1" to exclude everyone with flag 1 or "1|7|3" to exclude
                everyone with flags 1, 7 or 3.

                Leaving any SKIP* empty will speed it up. Each check takes extra time.
                If the user, group or flag is excluded, this script will allow everything.

NUKE_PREFIX   = If you do not plan on using the dirloglist_gl binary to check if a release was 
                previously on site or was nuked, you can use NUKE_PREFIX to at least check
                if the release is already nuked in the same dir. Just add the prefix
                that your nuked releases get, for example: "_NUKED-"
                and they will be blocked from being created.

                If you DO plan on using dirloglist_gl (see below), then set this one 
                to "".

                Yes, you can have both, but theres no point and will only slow it down.


If you did the dirloglist_gl part above, the following settings are for the functionality
of checking if the dir was uploaded before. If all you want is case matching block, then
you only need to check ERROR1 below too. 

DIRLOGLIST_GL  = Where is dirloglist_gl? default is /bin/dirloglist_gl
                 If you dont want to use this functionality, set it to "". If so, you
                 done need the dirloglist_gl binary.

SKIPDIRS       = These dirs will not be checked if a user try to create them.
                 We dont want to block creation of CD1, Sample, etc. 
                 Its a standard egrep line so here are some explanations. Note: nothing 
                 is case sensitive:

                 ^sample$    = This means the entire dir is called sample. Nothing more
                               and nothing less. ^ forces start only and $ forces end only.

                 ^vobsub.*   = This means the dir must start with vobsub and can end with anything
                               .* = Anything after.

                 ^xvid\.decoder.* = A dot in a standard egrep line means "anything", so do specify
                                    a real dot, we must escape it with a \ first, ie \.
                                    You should do the same for ALL non alpha chars.

                 ^CD[1-9]    = The dir starts with CD and can then contain 1-9 after, so this one
                               catches CD1 -> CD9.
                               Likewise, ^CD[0-9][0-9] matches CD01 -> CD99

                 If we were to skip the ^ and $ in sample (for instance), then something like
                 We.Repacked.The.Sample-OstGroup would be excluded. Dont want that.

ALLOWFILE      = This is a file. If the exact releasename is in this file, it will NOT be blocked by
                 the "previously uploaded part" of this script. 

                 I've seen that happen sometimes. You wipe a directory and someone is going to 
                 upload that dir again = no go. Now you can just
                 add the releasename to this file and it goes through anyway.

                 If you use this feature, create that file and set chmod 666 on it.
                 Remember that the path is seen chrooted, so its actually /glftpd/tmp.

                 Also, you can load tur-dirallow.tcl to your bot to allow dirs from irc.
                 Edit that file for more info on it.

                 Note that only the latest allowed dir is stored in this file. Once the dir
                 is created, there is no use in keeping it in there as it would just grow and grow
                 thus slowing down this script when checking it.

Setting DIRLOGLIST_GL will make it a bit slower when creating dirs. How much depends on the
size of the dirlog and machine power. Up to you to use it or not.

On to some more generic setups.

DENYSAMENAMEDIRS = This will prevent any directory name that currently exists in the parent path from 
                   being created, such as the directory /site/Testgroup/Ost, it will block creation of 
                   "testgroup" or "ost".
                   It is not case sensitive.
                   Set to TRUE or FALSE.

DENYGROUPS     = This is for banning groups. Format:
                 Path:group1|group2|etc

                 Escape any non alpha chars (like . is \. and - is \- etc).
                 Groups are a standard egrep -i line (non case sensitive).

                 Set to "" to disable.

                 Example:
                 DENYGROUPS="
                 /site:\-BadGroup$
                 /site/SVCD:\-SMB$
                 /site/DIVX:\-BP$
                 "

                 That will deny releases from BadGroup everywhere, SMB in SVCD and BP in DIVX.
              
                 \-SMB$ means it must end with -SMB to deny it. This is what you'll use for
                 groupnames.

                 Seperate each "word" with a |

                 To specify a space, use "\ " without the quotes (just escape it).

DENYDIRS       = This is the the same as DENYGROUPS but with a different error output.
                 ^NUKED means it must START with nuked (reacts on NUKED-Movie but not Movie-NUKED).

ALLOWDIRS      = A reversed DENYGROUPS and DENYDIRS.. This is for those sections where you 
                 only allow certain types of releases/groups.
                 Format: /path/to/section:What_To_Allow
                 Example:

                 ALLOWDIRS="
                 /site/MP3:\-BPM$|\-Lth$|[\_-\.]OST[\_-\.]
                 "

                 This would allow only releases from BPM, Lth or if it contains OST in the
                 /site/MP3 section.

                 Its not case sensitive.

ALLOWDIRS_OVERRULES_DENYGROUPS = TRUE/FALSE. If this is set to TRUE, it will override DENYGROUPS
                                 should it be found as an alloweddir in ALLOWDIRS. If FALSE, it will
                                 still block creation of the dir, should it be a banned group.

ALLOWDIRS_OVERRULES_DENYDIRS   = TRUE/FALSE. Same as above, but if TRUE, it will override DENYDIRS
                                 if its found in that one.
                                 As above, if this is FALSE, it will still be blocked, even if found
                                 in ALLOWDIRS.


Now for the error outputs. $1 means the dir the user tried to create. $2 is the dir he tried
to create it in.

ERROR1   = This is the error message the user gets if he tries to create a dir that is already there
           but with another case. For instance, trying to create Sample when SAMPLE is already there.

ERROR2   = This is the error the user gets if the dir was already created before.

ERROR3   = This is the error when a user tried to create a dir who got denied cause of DENYGROUPS.

ERROR4   = This is the error when a user tried to create a dir who got denied cause of DENYDIRS.

ERROR5   = This is the error when a user tried to create a dir who got denied cause of ALLOWDIRS.

ERROR6   = This is the error when a user tried to create a dir blocked by DENYSAMENAMEDIRS=TRUE

DEBUG    = TRUE/FALSE. If this is TRUE, it will show in glftpd what its doing and why stuff fail.
           It also tests if the strings binary works by displaying the 5 last entries in it.

GLLOG    = If you want bot output when someone is an idiot, set path to glftpd.log here, seen
           chrooted of course.
           Setting this to "" disables all "IERROR?" outputs.

BOLD     = Just so we can use $BOLD in irc output. Dont touch it.

IERROR1  = What to say to irc on ERROR1. If you dont want announce for this particular error, set
           it to "" or put a # infront of it.

IERROR2  = Same
IERROR3  = Same
IERROR4  = Same
IERROR5  = Same
IERROR6  = Same

Bot announce is by default for dZSbot.tcl. If you use another botscript, look up proc_announce()
in this script and change the echo line to $GLLOG to suit your botscript. The text its saying
is in variable $OUTPUT.

It uses the TURGEN trigger to announce which is the same as some of my other scripts, so if you
already have that added, skip this next part, otherwise do this in dZSbot.tcl:

#--
 To 'set msgtypes(DEFAULT)', add TURGEN

 Add the following in the appropriate places:
 set chanlist(TURGEN)  "#YourChan"
 set disable(TURGEN)   0
 set variables(TURGEN) "%msg"
 set announce(TURGEN)  "%msg"
#--

Hint: For SS5, I'm told you can replace TURGEN: with RAW: and it should work
without doing anything else.

Hint2: For pzs-ng beta3+, its the same setup, part from set announce(TURGEN).
Instead of the set announce above, you'll want to add
announce.TURGEN = "%msg"
in your theme file for the bot.

--[ Info ]-----------------------------------------------------------

Note that none of the errors will appear if they try to create the dir when it is there with the same
name. glftpd wont allow that anyway so better it handles that.

For example, someone tries to create /site/SVCD/Some.Movie-OST, but that dir is already there with the
exact same case. If so, it will just quit and allow it. Since everyone just drags the release they want
to move right over, it will try to create the dir and fail. That has always been the case. Just that we
dont want them getting a "That dir is already there, idiot" message when doing so.

It also speeds it up by not having to do all the checks if the dir is already there.


Thats all there is to it. Have fun =)
http://www.grandis.nu/glftpd
