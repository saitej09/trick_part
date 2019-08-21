/************************************************************************
PURPOSE: (Represent the state and initial conditions of an http server)
LIBRARY DEPENDENCIES:
    ((simpleJSON.o)
     (VariableServerVariable.o)
    )
**************************************************************************/
#include <string>
#include <sstream>
#include <iomanip> // for setprecision
#include "trick/memorymanager_c_intf.h"
#include "trick/input_processor_proto.h"
#include "trick/exec_proto.h"
#include "../include/VariableServerSession.hh"
#include "../include/simpleJSON.hh"


// CONSTRUCTOR
VariableServerSession::VariableServerSession( struct mg_connection *nc ) : WebSocketSession(nc) {
    intervalTimeTics = exec_get_time_tic_value(); // Default time interval is one second.
    nextTime = LLONG_MAX;
    cyclicSendEnabled = false;
}

// DESTRUCTOR
VariableServerSession::~VariableServerSession() {
    clear();
}

// Base class virtual function.
void VariableServerSession::stageData() {
    long long simulation_time_tics = exec_get_time_tics();
    if ( cyclicSendEnabled && ( simulation_time_tics >= nextTime )) {
        stageVariableValues();
    }
    nextTime = (simulation_time_tics - (simulation_time_tics % intervalTimeTics) + intervalTimeTics);
}

// Base class virtual function.
void VariableServerSession::sendMessage() {
    std::vector<VariableServerVariable*>::iterator it;
    std::stringstream ss;

    ss << "{ \"msg_type\" : \"values\",\n";
    ss << "  \"time\" : " << std::setprecision(16) << stageTime << ",\n";
    ss << "  \"values\" : [\n";

    for (it = sessionVariables.begin(); it != sessionVariables.end(); it++ ) {
        if (it != sessionVariables.begin()) ss << ",\n";
        (*it)->writeValue(ss);
     }
     ss << "]}" << std::endl;
    std::string tmp = ss.str();
    const char * message = tmp.c_str();
    mg_send_websocket_frame(connection, WEBSOCKET_OP_TEXT, message, strlen(message));
}

// Base class virtual function.
int VariableServerSession::handleMessage(std::string client_msg) {

     int status = 0;
     std::vector<Member*> members = parseJSON(client_msg.c_str());
     std::vector<Member*>::iterator it;
     std::string cmd;
     std::string var_name;
     std::string pycode;
     int period;

     for (it = members.begin(); it != members.end(); it++ ) {
         if (strcmp((*it)->key, "cmd") == 0) {
             cmd = (*it)->valText;
         } else if (strcmp((*it)->key, "var_name") == 0) {
             var_name = (*it)->valText;
         } else if (strcmp((*it)->key, "period") == 0) {
             period = atoi((*it)->valText);
         } else if (strcmp((*it)->key, "pycode") == 0) {
             pycode = (*it)->valText;
         }
     }

     if (cmd.empty()) {
         printf ("No \"cmd\" member found in client message.\n");
         status = 1;
     } else if (cmd == "var_add") {
         addVariable( strdup( var_name.c_str()));
     } else if (cmd == "var_cycle") {
         setTimeInterval(period);
     } else if (cmd == "var_pause") {
         pause();
     } else if (cmd == "var_unpause") {
         unpause();
     } else if (cmd == "var_send") {
         stageVariableValues();
         sendMessage();
     } else if (cmd == "var_clear") {
         clear();
     } else if (cmd == "var_exit") {
         //TODO
         // nc->flags |= MG_F_SEND_AND_CLOSE;
     } else if (cmd == "python") {
         // Remove carriage-returns from pycode.
         pycode.erase(std::remove(pycode.begin(), pycode.end(), '\r'), pycode.end());
         // Call the Trick input processor.
         ip_parse(pycode.c_str());
     } else {
         sendErrorMessage("Unknown Command: \"%s\".\n", cmd.c_str());
         status = 1;
     }
     return status;
}

void VariableServerSession::setTimeInterval(unsigned int milliseconds) {
    intervalTimeTics = exec_get_time_tic_value() * milliseconds / 1000;
}

void VariableServerSession::addVariable(char* vname){
    REF2 * new_ref ;
    new_ref = ref_attributes(vname);
    if ( new_ref == NULL ) {
        sendErrorMessage("Variable Server could not find variable %s.\n", vname);
        new_ref = make_error_ref(vname);
    } else if ( new_ref->attr ) {
        if ( new_ref->attr->type == TRICK_STRUCTURED ) {
            sendErrorMessage("Variable Server: var_add cant add \"%s\" because its a composite variable.\n", vname);
            free(new_ref);
            new_ref = make_error_ref(vname);

        } else if ( new_ref->attr->type == TRICK_STL ) {
            sendErrorMessage("Variable Server: var_add cant add \"%s\" because its an STL variable.\n", vname);
            free(new_ref);
            new_ref = make_error_ref(vname);
        }
    } else {
        sendErrorMessage("Variable Server: BAD MOJO - Missing ATTRIBUTES.");
        free(new_ref);
        new_ref = make_error_ref(vname);
    }

    if ( new_ref != NULL ) {
        // This REF2 object will "belong" to the VariableServerSessionVariable, so it has
        // the right and responsibility to free() it in its destructor.
        VariableServerVariable *sessionVariable = new VariableServerVariable( new_ref ) ;
        sessionVariables.push_back( sessionVariable ) ;
    }
}

void VariableServerSession::stageVariableValues() {
    stageTime = (double)exec_get_time_tics() / exec_get_time_tic_value();
    std::vector<VariableServerVariable*>::iterator it;
    for (it = sessionVariables.begin(); it != sessionVariables.end(); it++ ) {
        (*it)->stageValue();
    }
}

void VariableServerSession::pause()   { cyclicSendEnabled = false; }

void VariableServerSession::unpause() { cyclicSendEnabled = true;  }

void VariableServerSession::clear() {
        std::vector<VariableServerVariable*>::iterator it;
        it = sessionVariables.begin();
        while (it != sessionVariables.end()) {
            delete *it;
            it = sessionVariables.erase(it);
        }
}

void VariableServerSession::exit() {}

int VariableServerSession::bad_ref_int = 0 ;

#define MAX_MSG_SIZE 4096
int VariableServerSession::sendErrorMessage(const char* fmt, ... ) {
    char errText[MAX_MSG_SIZE];
    char msgText[MAX_MSG_SIZE];
    va_list args;

    errText[0]=0;
    msgText[0]=0;

    va_start(args, fmt);
    (void) vsnprintf(errText, MAX_MSG_SIZE, fmt, args);
    va_end(args);

    sprintf(msgText, "{ \"msg_type\" : \"error\",\n"
                     "  \"error\" : \"%s\"}\n", errText);

    mg_send_websocket_frame(connection, WEBSOCKET_OP_TEXT, msgText, strlen(msgText));
    return (0);
}

REF2* VariableServerSession::make_error_ref(const char* in_name) {
    REF2* new_ref;
    new_ref = (REF2*)calloc(1, sizeof(REF2));
    new_ref->reference = strdup(in_name) ;
    new_ref->units = NULL ;
    new_ref->address = (char *)&bad_ref_int ;
    new_ref->attr = (ATTRIBUTES*)calloc(1, sizeof(ATTRIBUTES)) ;
    new_ref->attr->type = TRICK_NUMBER_OF_TYPES ;
    new_ref->attr->units = (char *)"--" ;
    new_ref->attr->size = sizeof(int) ;
    return new_ref;
}
