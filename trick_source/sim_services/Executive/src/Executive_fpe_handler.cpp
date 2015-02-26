/*
   PURPOSE: (Floating point exception handler)

   PROGRAMMERS: (((Robert W. Bailey) (LinCom) (April 1992) (--) (Realtime)))
 */

/*
 * $Id: fpe_handler.cpp 775 2010-06-29 23:04:51Z lin $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

/*
 * FPE specific headers
 */
#ifdef __linux
#include <fenv.h>
#endif

#include "sim_services/Executive/include/Executive.hh"
#include "sim_services/Executive/include/exec_proto.h"
#include "sim_services/Message/include/message_proto.h"

/**
@design
-# This signal handler processes SIGFPE
-# Write a message to stderr that a signal has been caught.
-# In Linux print out the type of floating point error that occured.
-# If attach_debugger == true, attempt to attach a debugger to the current process.
-# Else if stack_trace == true, attempt to attach a debugger to print a stack trace.
-# Exit the process with a -2 return.
*/

void Trick::Executive::fpe_handler(siginfo_t * sip __attribute__((unused)) ) {

    write( 2 , "\033[31mProcess terminated by signal FPE" , 36 ) ;
#if __linux
    /* Determine what floating point error occurred */
    if (sip != (siginfo_t *) NULL) {
        switch (sip->si_code) {
            case FPE_FLTRES:          /* inexact */
                write( 2, " (inexact)", 10 );
                return ;
            case FPE_FLTUND:          /* underflow */
                write( 2, " (floating point underflow)", 27 );
                break;
            case FPE_FLTINV:          /* invalid operand */
                write( 2, " (invalid operand)", 18 );
                break;
            case FPE_FLTDIV:          /* division-by-zero */
                write( 2, " (floating point division-by-zero)", 34);
                break;
            case FPE_FLTOVF:          /* overflow */
                write( 2, " (floating point overflow)", 26);
                break;
            case FPE_INTOVF:          /* integer overflow */
                write( 2, " (integer overflow)", 19);
                break;
            case FPE_INTDIV:          /* integer division-by-zero */
                write( 2, " (integer division-by-zero)", 27);
                break;
            default:
                break;
        }
    }
#endif
    write( 2 , "\033[0m\n" , 5 ) ;

    /*
     Attempt to attach with debugger or print stack trace.  Not a requirement.
     sprintf and system are not async signal safe, but we don't have anything to lose.
     */
#if __linux
    char command[1024];
    if (attach_debugger == true) {
        sprintf(command, "%s -silent /proc/%d/exe %d", debugger_command.c_str(), getpid(), getpid());
        system(command);
    } else if (stack_trace == true ) {
        sprintf(command, "%s -silent -batch -x ${TRICK_HOME}/bin/gdb_commands "
                "/proc/%d/exe %d | grep -A 20 \"signal handler\"", debugger_command.c_str(), getpid(), getpid());
        system(command);
    }
#elif __APPLE__
    char command[2048];
    char path[1024] ;
    uint32_t size = sizeof(path) ;
    if (_NSGetExecutablePath(path, &size) == 0 ) {
        if (attach_debugger == true) {
            write( 2 , "Attempting to attach debugger... standby.\n" , 41 ) ;
            sprintf(command, "%s -silent %s %d", debugger_command.c_str(), path, getpid());
            system(command);
        } else if (stack_trace == true ) {
            write( 2 , "Attempting to generate stack trace... standby.\n" , 47 ) ;
            sprintf(command, "%s -batch -x ${TRICK_HOME}/bin/gdb_commands "
                    "%s %d", debugger_command.c_str(), path, getpid());
            system(command);
        }
    }
#endif

    // The user has chosen for Trick to intervene on their behalf.  Exit if handler called.
    _exit(-2) ;

}
