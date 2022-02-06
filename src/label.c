/*  Label
 */

#define VERSION          "1.5"

/*

    Modified by Joe Cosentino 2000,2003.
    Modified by Brian E. Reifsnyder, August 2000.
    Modified by Eric Auer 2003.
    Modified 2021 (see history)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

/* Many DOS functions are called with only the pointer offset set */
#if !defined(__TINY__) && !defined(__SMALL__)
#error Must be compiled with the TINY or SMALL model.
#endif

/* I N C L U D E S /////////////////////////////////////////////////////// */

#include <dos.h>
#ifdef __TURBOC__
#include <dir.h>   /* findfirst, getdisk, setdisk, some constants */
                   /* (others are in dos.h */
#endif
#include <ctype.h> /* toupper */
#include <stdlib.h>
#include <string.h>

#include "../kitten/kitten.h"

#if 1
int Xprintf(const char * fmt, ...);
int Xsprintf(char *, const char * fmt, ...);
#define PRINTF Xprintf
#define SPRINTF Xsprintf
#else
#include <stdio.h>
#define PRINTF printf
#define SPRINTF sprintf
#endif

/* D E F I N E S ///////////////////////////////////////////////////////// */

#define BAD_CHARS        21
#define MAX_LABEL_LENGTH 11
#define fcbDRIVE         7
#define NAME             8
#define ENDNAME          19
#define SPACEBAR         ' '
#define DBQ              '"'
#define REMOTE           0x9
#define IOCTL            0x44
#define PATHLEN          64

/* G L O B A L S ///////////////////////////////////////////////////////// */

char bad_chars[BAD_CHARS] = "*?/\\|.,;:+=<>[]()&^\"";
char Drive[3] = "?:";
char TempDrive[3] = "?:";
char Label[13] = "";
char OldLabel[13] = "";
char newline[3] = "\r\n";
char curdir[PATHLEN] = {0};
char rootdir[4] = "?:\\";
char getsbuf[MAX_LABEL_LENGTH+3];
signed char input_redir = -1;
int  NoLabel = 1;
int  firsttime = 1;
int  DriveNotFirst = 0;
char fcb[128] =      {0xff, 0, 0, 0, 0, 0, 0x8, 0,
  '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?'};
char con_fcb[44] =   {0xff, 0, 0, 0, 0, 0, 0, 0,
  'C', 'O', 'N',' ',' ',' ',' ',' ',' ',' ',' '};
char creat_fcb[44] = {0xff, 0, 0, 0, 0, 0, 0x8, 0,
  ' ', ' ', ' ',' ',' ',' ',' ',' ',' ',' ',' '};

nl_catd cat;    /* handle for localized messages (catalog) */

/* P R O T O T Y P E S /////////////////////////////////////////////////// */

int classify_args(int, char *[], char *[], char *[]);
void mygets(char *, unsigned int);
#ifdef __TURBOC__
void interrupt ctrlc_hndlr(void);
#else
void __far __interrupt ctrlc_hndlr(void);
#endif
int valid_drive(char *);
void GetDrive(void);
void get_label(void);
void disp_label(void);
int make_label(void);
void del_label(void);
void save_label(char *);
int check_label(char *);
int valid_label(char *);
void do_cmdline(int, char *[]);
int check_quotes(void);
void on_exit_hook(void);

/* F U N C T I O N S ///////////////////////////////////////////////////// */

int classify_args(int narg, char *rawargs[], char *fileargs[], char *optargs[])
{
    int index, jndex, kndex;
    char *argptr;

    for (index=0,jndex=0,kndex=0;index<narg;index++)
        {
        argptr = rawargs[index];
        if (*argptr == '/')
            {
            argptr++;
            optargs[kndex++] = argptr;
            } /* end if. */
        else
            {
            fileargs[jndex++] = argptr;
            } /* end else. */
            
        } /* end for. */

   return kndex;

} /* end classify_args. */

/* //////////////////////////////////////////////////////////////////////// */

void GetCurDir(char *dir)
{
    union REGS regs;

    regs.x.ax = 0x4700;
    regs.h.dl = 0;
    regs.x.si = (unsigned) dir; /* needs NEAR POINTER */
    intdos(&regs, &regs);

} /* end GetCurDir. */

/* /////////////////////////////////////////////////////////////////////// */

void SetCurDir(char *dir)
{
    union REGS regs;

    regs.x.ax = 0x3B00;
    regs.x.dx = (unsigned) dir; /* needs NEAR POINTER */
    intdos(&regs,&regs);

} /* end SetCurDir. */

/* /////////////////////////////////////////////////////////////////////// */

void mygets(char *buff, unsigned int length)
{
    char *sptr;
    int i;
    union REGS regs;

    if (input_redir==-1)
        {
        regs.h.ah = 0x44;
        regs.h.al = 0x0;
        regs.x.bx = 0x0;
        intdos(&regs, &regs);
        if (regs.x.dx & 0x80)
            input_redir = 0;
        else
            input_redir = 1;

        } /* end if. */

    if (input_redir)
        {
        regs.h.ah = 0xb; /* check if input waiting */
        intdos(&regs,&regs);
        if (regs.h.al==0) /* no input waiting */
            {
            /* puts(""); */ /* WHY ??? */
            exit(10);
            } /* end if. */

        } /* end if. */

    getsbuf[0] = (char) length;
    getsbuf[2] = 0;
    regs.x.dx = (unsigned) getsbuf;
    regs.h.ah = 0xa;
    intdos(&regs,&regs);
    PRINTF("\n"); /* puts("\r"); */
    sptr = &getsbuf[2];
    if (*sptr==0x1a)
        {
        /* puts(""); */ /* WHY ??? */
        exit(10);
        } /* end if. */

    for (i=0;i<getsbuf[1];i++)
        {
        *buff = *sptr;
        buff++;
        sptr++;
        } /* end for. */

    *buff='\0';

} /* end mygets. */

/* /////////////////////////////////////////////////////////////////////// */

#ifdef __TURBOC__
void interrupt ctrlc_hndlr()
#else
void __far __interrupt ctrlc_hndlr()
#endif
{
    exit(0); /* we can ignore the Turbo C suggestion for interrupt, */
             /* to turn off the stack warning register variables, right? */

} /* end ctrlc_hndlr. */

/* /////////////////////////////////////////////////////////////////////// */

int valid_drive(char *s)
{
    char buf1[128],buf2[128];
    struct SREGS sregs;
    union REGS regs;

    if (s[1] != ':')
        return(0);

    *TempDrive = *s;

    /* Make sure this is a valid drive. */
    con_fcb[fcbDRIVE] = *TempDrive-'A'+1;
    regs.x.ax = 0x0f00;
    regs.x.dx = (unsigned)con_fcb; /* needs NEAR POINTER */
    intdos(&regs, &regs);
    if (regs.h.al)
        {
        PRINTF(catgets(cat, 2, 5, "Not a valid drive\n"));
        exit(1);
        } /* end if. */

    regs.x.ax = 0x1000;
    regs.x.dx = (unsigned)con_fcb; /* needs NEAR POINTER */
    intdos(&regs, &regs);                 /* Now close the file. */

    /* Make sure user is not trying to label a network drive. */
    regs.h.ah = IOCTL;
    regs.h.al = REMOTE;
    regs.h.bl = (char)(*TempDrive-'A'+1);
    intdos(&regs, &regs);
    if (regs.x.dx & 0x1000)
        {
        PRINTF(catgets(cat, 2, 6, "You cannot label a network drive\n"));
        exit(5);
        } /* end if. */

    /* Make sure the user is not trying to label a drive which has */
    /* been ASSIGNed, JOINed, or SUBSTed. */
    strcpy(buf1, TempDrive);
    strcat(buf1, "\\");
    segread(&sregs);
    #ifdef __DMC__
    regs.x.si = (unsigned)buf1; /* needs NEAR POINTER */
    regs.x.di = (unsigned)buf2; /* needs NEAR POINTER */
    #else
    sregs.ds = FP_SEG(buf1);
    regs.x.si = FP_OFF(buf1);
    sregs.es = FP_SEG(buf2);
    regs.x.di = FP_OFF(buf2);
    #endif
    regs.x.ax = 0x6000;
    intdosx(&regs, &regs, &sregs);
    if (*buf1 != *buf2)
        {
        PRINTF(catgets(cat, 2, 7, "You cannot label a drive which has\n"
                                  "been ASSIGNed, JOINed, or SUBSTed.\n"));
        exit(5);
        } /* end if. */

    return(1);

} /* end valid_drive. */

/* /////////////////////////////////////////////////////////////////////// */

void GetDrive()
{
    unsigned driveno;

#ifdef __TURBOC__
    driveno = getdisk(); /* get current drive */
    Drive[0] = 'A' + driveno;
#else
    _dos_getdrive(&driveno);
    Drive[0] = 'A' + driveno - 1;
#endif
    valid_drive(Drive);

} /* end GetDrive. */

/* /////////////////////////////////////////////////////////////////////// */

void get_label()
{ 
    char temp[MAX_LABEL_LENGTH+1]; 

    do
        {
        PRINTF(catgets(cat, 1, 1, "Volume label (11 characters, ENTER for none)? "));
        mygets(temp,MAX_LABEL_LENGTH+1);
        } /* end do. */
    while (check_label(temp));
    strcpy(Label, temp);

} /* end get_label. */

/* /////////////////////////////////////////////////////////////////////// */

void disp_label()
{
/*
Offset  Size    Description     (Table 01766)
00h    WORD    0000h (info level)
02h    DWORD   disk serial number (binary)
06h 11 BYTEs   volume label or "NO NAME    " if none present
11h  8 BYTEs   (AL=00h only) filesystem type (see #01767)
*/
    struct {
      short int level;
      unsigned short int lo_serno;
      unsigned short int hi_serno;
      char label[11];
      char fstype[8];
    } dinfo;

    union REGS regs;
    struct SREGS sregs;

    /* First set the dta to be fcb so information returned is put there. */
    regs.x.ax = 0x1a00;
    regs.x.dx = (unsigned)fcb; /* needs NEAR POINTER */
    intdos(&regs, &regs);

    /* Now try to find the volume label. */
    fcb[fcbDRIVE] = *Drive-'A'+1;
    regs.x.ax = 0x1100;
    regs.x.dx = (unsigned)fcb; /* needs NEAR POINTER */
    intdos(&regs, &regs);
    if (regs.h.al)
        {
        PRINTF(catgets(cat, 1, 2, "Volume in drive %c has no label\n"), *Drive);
        } /* end if. */
    else
        {
        NoLabel = 0;
        fcb[ENDNAME] = '\0';
        PRINTF(catgets(cat, 1, 3, "Volume in drive %c is %s\n"), *Drive, &fcb[NAME]);
        } /* end else. */

    /* Now print out the volume serial number, if it exists. */
    segread(&sregs);
    regs.x.ax = 0x6900;
    regs.h.bl = *Drive-'A'+1;
    regs.x.dx = (unsigned)&dinfo; /* needs NEAR POINTER */
    intdosx(&regs, &regs, &sregs);
    if (!regs.x.cflag)
        {
        PRINTF(catgets(cat, 1, 4, "Volume serial number is %04X-%04X\n"), dinfo.hi_serno, dinfo.lo_serno);
        } /* end if. */

} /* end disp_label. */

/* /////////////////////////////////////////////////////////////////////// */

int make_label()
{
    union REGS regs;
    int i,length;

    creat_fcb[fcbDRIVE] = *Drive-'A'+1;
    length = strlen(Label);
    for (i=0;i<length;i++)
        creat_fcb[NAME+i] = Label[i];

    regs.x.ax = 0x1600;                  /* Create the file. */
    regs.x.dx = (unsigned)creat_fcb; /* needs NEAR POINTER */
    intdos(&regs, &regs);
    if (!regs.h.al)
        {
        regs.x.ax = 0x1000;              /* Close the file. */
        regs.x.dx = (unsigned)creat_fcb; /* needs NEAR POINTER */
        intdos(&regs, &regs);
        return(1);
        } /* end if. */
    else
        return(3);

} /* end make_label. */

/* /////////////////////////////////////////////////////////////////////// */

void save_label(char *string)
{
    if (firsttime)
        firsttime=0;
    else
        {
        if (DriveNotFirst)
            {
            PRINTF(catgets(cat, 2, 8, "Invalid drive\n"));
            exit(4);
            } /* end if. */
        else
            strcat(Label, " ");
        } /* end else. */

    strcat(Label,string);

} /* end save_label. */

/* /////////////////////////////////////////////////////////////////////// */

void del_label()
{
    char findstring[7];
#ifdef __TURBOC__
    struct ffblk findstr;
#else
    struct find_t findstr;
#endif
    union REGS regs;

    strcpy(findstring, "?:\\*.*");
    *findstring = *Drive;
#ifdef __TURBOC__
    if (!findfirst(findstring, &findstr, FA_LABEL))
    /* findfirst and FA_... defined in dir.h and dos.h */
        {
        strcpy(OldLabel, findstr.ff_name);
#else
    if (!_dos_findfirst(findstring, _A_VOLID, &findstr))
        {
        strcpy(OldLabel, findstr.name);
#endif
        fcb[fcbDRIVE] = *Drive-'A'+1;
        regs.x.ax = 0x1300;
        regs.x.dx = (unsigned)fcb; /* needs NEAR POINTER */
        intdos(&regs, &regs);
        } /* end if. */

} /* end del_label. */

/* /////////////////////////////////////////////////////////////////////// */

int check_label(char *s)
{
    int length,LabelLen;
    int i,j;
    char * sup;
    char *msg;

    /* s = strupr(s); */
    sup = s;
    while (*sup != '\0')
        {
        *sup = toupper(*sup);
        sup++;
        } /* end while */

    /* Make sure label is not too long. */
    length = strlen(s);
    LabelLen = strlen(Label);
    if ((length > MAX_LABEL_LENGTH) ||
        ((LabelLen > 0) && ((length + LabelLen + 1) > MAX_LABEL_LENGTH)))
        {
        strcpy(Label,"");
        PRINTF(catgets(cat, 2, 1, "The label is too long. The label must\n"
                                  "be 11 characters or less.\n"));
        return (2);
        } /* end if. */

    msg = "Invalid volume label\n";

    /* Make sure all characters are legitimate. */
    for (i=0;i<length;i++)
        {
        for (j=0;j<BAD_CHARS;j++)
            {
            if (s[i]==bad_chars[j])
                {
                strcpy(Label,"");
                PRINTF(catgets(cat, 2, 2, msg));
                return(4);
                } /* end if. */

            } /* end for. */

        if ((unsigned char)s[i]<(unsigned char)SPACEBAR)
            {
            strcpy(Label,"");
            PRINTF(catgets(cat, 2, 2, msg));
            return(4);
            } /* end if. */

        } /* end for. */

    return(0);

} /* end check_label. */

/* /////////////////////////////////////////////////////////////////////// */

int valid_label(char *s)
{
    if (s[2] != '\0')
        {
        if (check_label(&s[2]))
            return(1);

        save_label(&s[2]);
        return(0);
        } /* end if. */

    return 0; /* empty string is not a valid label, or is it??? */

} /* end valid_label. */

/* /////////////////////////////////////////////////////////////////////// */

void do_cmdline(int ac, char *av[])
{
    int i;

    for (i=1;i<ac;i++)
        {
        if (valid_drive((av[i])=strupr(av[i])))
            {
            if (*Drive == '?')
                {
                *Drive = *av[i];        /* Save the drive letter. */
                } /* end if. */
            else
                {
                PRINTF(catgets(cat, 2, 3, "There were multiple drives mentioned.\n"
                                          "Please select one drive to label at a time.\n"));
                exit(26);
                } /* end if. */
      
            /* See if the drive letter is the first parameter. */
            if (i != 1)
                DriveNotFirst = 1;

            /* See if the label is tacked right onto the drive letter. */
            /* Verify label if it is. */
            if (valid_label(av[i]))
                return;

            } /* end if. */
        else
            {
            if (check_label(av[i])) 
                return;
            else
                save_label(av[i]);

            } /* end else. */

        } /* end for. */

    if (check_quotes())
        {
        strcpy(Label,"");
        PRINTF(catgets(cat, 2, 4, "Invalid label\n"));
        return;
        } /* end if. */

} /* end process_cmdline. */

/* /////////////////////////////////////////////////////////////////////// */

int check_quotes()
{
    union REGS regs;
#ifdef __TURBOC__
    unsigned char far *psp;
#else
    unsigned char __far *psp;
#endif

    /* Get the segment address of PSP. */
    regs.x.ax = 0x6200;
    intdos(&regs, &regs);

    /* Get address of original command line. */
    psp = MK_FP(regs.x.bx, 0x81);

    /* Check for double quotes. */
    for (;*psp != '\r';psp++)
        {
        if (*psp==DBQ)
            return(1);

        } /* end for. */

    return(0);

} /* end check_quotes. */

/* /////////////////////////////////////////////////////////////////////// */

void on_exit_hook()
{

    if (curdir[0] != 0)            /* Originally at root? */
        SetCurDir(curdir);        /* Change to original dir. */

    catclose(cat);

} /* end on_exit. */

/* /////////////////////////////////////////////////////////////////////// */

int main(int argc, char *argv[])
{
    char ans[2];
    char *fileargs[64];
    char *optargs[64];
    int n_options;
    int index;
    int help_flag = 0;

    cat = catopen("label", 0);

#ifdef __TURBOC__
    setvect(0x23, ctrlc_hndlr);
#else
    _dos_setvect(0x23, ctrlc_hndlr);
#endif
    atexit(on_exit_hook);
    n_options = classify_args(argc, argv, fileargs, optargs);
    for (index=0;index<n_options;index++)
        {
        if (optargs[index][0] == '?')
            help_flag=1;
        else
            {
            PRINTF(catgets(cat, 2, 0, "Invalid parameter - /%c\n"), optargs[index][0]);
            exit(1);
            } /* end else. */

        } /* end for. */

    if (help_flag)
        {
        PRINTF("\n");
        PRINTF(catgets(cat, 0, 0, "LABEL Version %s\n"), VERSION);
        PRINTF(catgets(cat, 0, 1, "Creates, changes or deletes the volume label of a disk.\n"));
        PRINTF("\n");
        PRINTF(catgets(cat, 0, 2, "Syntax: LABEL [drive:][label] [/?]\n"));
        PRINTF(catgets(cat, 0, 3, "  [drive:]  Specifies which drive you want to label\n"));
        PRINTF(catgets(cat, 0, 4, "  [label]   Specifies the new label you want to label the drive\n"));
        PRINTF(catgets(cat, 0, 5, "  /?        Displays this help message\n"));
        return 0;
        } /* end if. */

    do_cmdline(argc, argv);
    if (*Drive == '?')  /* If no drive specified, use current. */
        GetDrive();

    /* Save current directory and move to root. */
    GetCurDir(curdir);
    if (curdir[0] != 0)
        {
        *rootdir = *Drive;
        SetCurDir(rootdir);
        } /* end if. */
   
    /* If no label was specified, show current one first and then get new one. */
    if (*Label == '\0')
        {
        disp_label();
        get_label();
        } /* end if. */
    
    /* If they entered an empty label, then ask them if they want to */
    /* delete the existing volume label. */
    if ((*Label == '\0') && (!NoLabel))
        {
        const char *msg;
        const char *english_msg = "Delete current volume label (Y/N)? ";
        char Y, N;
#ifndef NOCATS
        const char *xlat_msg;
        char responses[2];

        xlat_msg = catgets(cat, 1, 0, english_msg);
        if (kitten_extract_response(xlat_msg, "(/)", responses, 2)) {
            Y = responses[0];
            N = responses[1];
            msg = xlat_msg;
        } else {
            PRINTF("Translation response letter parse error, please report\n");
#else
        {
#endif
            Y = 'Y';
            N = 'N';
            msg = english_msg;
        }

        do
            {
            PRINTF("\n");
            PRINTF(msg);
            mygets(ans,2); /* WHY not use getch? ??? */
            } /* end do. */
        while (((*ans=(char)toupper(*ans)) != Y) && (*ans != N));
    
        if (toupper(*ans) == N)
            exit(1);

        } /* end if. */

    /* Delete the old volume label. */
    del_label();
  
    /* Create the new one, if there is one to create. */
    if (*Label != '\0')
        {
        if (make_label())
            {
            exit(1);
            } /* end if. */

        } /* end if. */

    exit(0);
    return 0;

} /* end main. */


