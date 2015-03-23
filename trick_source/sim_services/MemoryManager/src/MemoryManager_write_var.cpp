#include <fstream>
#include <string.h>
#include "sim_services/MemoryManager/include/MemoryManager.hh"
#include "sim_services/Message/include/message_proto.h"
#include "sim_services/Message/include/message_type.h"

// GreenHills stuff
#if ( __ghs )
#include "ghs_stubs.h"
#endif


// MEMBER FUNCTION
void Trick::MemoryManager::write_composite_var( std::ostream& out_s,
                                                void*         address,
                                                ATTRIBUTES*   attr_list) {

    if (attr_list == NULL) {
        message_publish(MSG_ERROR, "Memory Manager ERROR: attr_list = NULL in write_composite_var.\n") ;
        return;
    }

    for (int ii = 0; attr_list[ii].name[0] != '\0'; ii++) {

        // If it's permitted to output the data type described by this ATTRIBUTE ...
        if (currentCheckPointAgent->output_perm_check(&attr_list[ii])) {
            void *elem_addr;
            if (attr_list[ii].mods & 2) { // This is a static member variable.
                elem_addr = (void*)attr_list[ii].offset;
            } else { // This is a normal member variable.
                elem_addr = (char*)address + (size_t)attr_list[ii].offset;
            }
            // Push the element name onto the name stack.
            currentCheckPointAgent->push_struct_elem( attr_list[ii].name);

            // Write the one or more assignment statements that represent the
            // values in this variable.
            write_var(out_s, elem_addr, &(attr_list[ii]));

            // Pop the element name from the name stack.
            currentCheckPointAgent->pop_elem();
        }
    }
    return;
}

// MEMBER FUNCTION
void Trick::MemoryManager::write_array_var( std::ostream& out_s,
                                            void*         address,
                                            ATTRIBUTES*   attr,
                                            int           curr_dim,
                                            int           offset) {

    if (attr == NULL) {
        message_publish(MSG_ERROR, "Memory Manager ERROR: attr = NULL in write_array_var().\n") ;
        return;
    }

    int array_element_count = attr->index[curr_dim].size;

    if (array_element_count == 0) { // This is a pointer (a.k.a: an unconstrained array).
        currentCheckPointAgent->assign_rvalue( out_s, address, attr, curr_dim, offset);
    } else { // This is a contrained array.

        // If this is an array of primitive-types and the user has not requested that we
        // write array in the expanded form  then write them more compactly.
        if ( (attr->type != TRICK_STRUCTURED ) && (expanded_arrays == false)) {
            currentCheckPointAgent->assign_rvalue( out_s, address, attr, 0, 0 );
        } else {

            // For each of the elements in the array ...
            for (int ii = 0; ii < array_element_count; ii++) {
                // Push the element index onto the name stack.
                currentCheckPointAgent->push_array_elem(ii);
                // If the current dimension is not the final dimension ...
                if (curr_dim < attr->num_index - 1) {
                    // The element itself is an array.
                    write_array_var( out_s, address, attr, curr_dim + 1, offset * array_element_count + ii);
                } else {
                    // The element itself is not an array.
                    if (attr->type == TRICK_STRUCTURED) { // The element is a composite.
                        char* elem_addr = (char*)address + (offset * array_element_count + ii) * attr->size ;
                        write_composite_var( out_s, elem_addr, (ATTRIBUTES*)attr->attr );
                    } else { // The element is a primitive.
                        int elem_offset = offset * array_element_count + ii;
                        currentCheckPointAgent->assign_rvalue( out_s, address, attr, curr_dim+1, elem_offset);
                    }
                }
                // Pop the element index back off of the name stack.
                currentCheckPointAgent->pop_elem();
            }
        }
    }
}

// MEMBER FUNCTION
void Trick::MemoryManager::write_var(std::ostream& out_s, void* address, ATTRIBUTES* attr) {

    if (attr->num_index > 0) {
        // This is an arrayed object.
        write_array_var( out_s, (char*)address, attr, 0, 0) ;
    } else {
        // This is not an arrayed object.
        if ( attr->type == TRICK_STRUCTURED ) {
            // This is a composite object.
            write_composite_var( out_s, (char*)address, (ATTRIBUTES*)(attr->attr)) ;
        } else {
            // This is a primitive object.
            currentCheckPointAgent->assign_rvalue( out_s, address, attr, 0, 0);
        }
    }
}

// MEMBER FUNCTION
void Trick::MemoryManager::write_var(std::ostream& out_s, ALLOC_INFO* alloc_info ) {

    ATTRIBUTES* reference_attr;
    reference_attr = make_reference_attr( alloc_info);

    // Push the basename onto the left-side name stack.
    currentCheckPointAgent->push_basename( alloc_info->name);

    write_var(out_s, (char*)(alloc_info->start), reference_attr);

    // Pop the basename that we pushed above.
    currentCheckPointAgent->pop_elem(); // Pop basename.

    free_reference_attr( reference_attr);
}

// MEMBER FUNCTION
void Trick::MemoryManager::write_var( std::ostream& out_s, const char* var_name ) {

    VARIABLE_MAP::iterator pos;
    ALLOC_INFO *alloc_info;

    pthread_mutex_lock(&mm_mutex);
    pos = variable_map.find( var_name);

    if (pos != variable_map.end()) {
        alloc_info = pos->second;
        write_var( out_s, alloc_info );
    } else {
        message_publish(MSG_ERROR, "Memory Manager ERROR: write_var(\"%s\") failed "
                                   "because the given name isn't known by the MemoryManager.", var_name) ;
    }
    pthread_mutex_unlock(&mm_mutex);
}

// MEMBER FUNCTION
void Trick::MemoryManager::write_var( std::ostream& out_s, void* address) {

    ALLOC_INFO *alloc_info;
    pthread_mutex_lock(&mm_mutex);
    alloc_info = get_alloc_info_of( address);

    if (alloc_info != NULL) {
        write_var( out_s, alloc_info );
    } else {
        message_publish(MSG_ERROR, "Memory Manager ERROR: write_var(%p) failed "
                                   "because no variable is being managed at that address.\n", address) ;
    }
    pthread_mutex_unlock(&mm_mutex);
}
