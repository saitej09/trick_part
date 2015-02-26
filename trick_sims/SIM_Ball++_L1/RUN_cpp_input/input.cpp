/*
 To use this as an input file include this file on the command line.
 S_main_${TRICK_HOST_CPU}.exe RUN_<dir>/input.cpp
 Trick will compile and link this file at simulation runtime.  The
 run_me function will automatically be executed during the
 input_processor phase of initialization.
*/

// need access to private/protected things? uncomment these.
//#define private public
//#define protected public

#include "../S_source.hh"

extern "C" int run_event_1() {
    message_publish(1, "event 1 fired!\n") ;
    return 0 ;
}

int run_event_2() {
    message_publish(1, "event 2 fired!\n") ;
    return 0 ;
}

int run_event_3() {
    message_publish(1, "event 3 fired!\n") ;
    return 0 ;
}

int run_event_4() {
    message_publish(1, "event 4 fired!\n") ;
    return 0 ;
}

int run_event_before() {
    message_publish(1, "event before fired!\n") ;
    return 0 ;
}

int run_event_before2() {
    message_publish(1, "event before2 fired!\n") ;
    return 0 ;
}

int run_event_after() {
    message_publish(1, "event after fired!\n") ;
    return 0 ;
}

int remove_events() {
    event_manager_remove_event(event_manager_get_event("event_3")) ;
    event_manager_remove_event(event_manager_get_event("event_before")) ;
    event_manager_remove_event(event_manager_get_event("event_after")) ;
    return 0 ;
}

extern "C" int run_me() {

    // modify variables within sim_objects.
    ball.obj.state.input.position[0] = 6.0 ;
    ball.obj.state.input.position[1] = 6.0 ;

    // add read events
    jit_add_read( 90.0 , run_event_1 ) ;
    jit_add_read( 80.0 , run_event_2 ) ;

    // add cyclic event
    jit_add_event( run_event_3 , "event_3" , 100.0 ) ;

    // events tied to jobs
    jit_add_event_before( run_event_before , "event_before", "ball.obj.state_print" ) ;
    jit_add_event_after( run_event_after, "event_after", "ball.obj.state_print" ) ;

    // add a read event that removes all other events.
    jit_add_read( 150.0 , remove_events ) ;

    exec_set_terminate_time(200.0) ;

    return 0 ;
}

