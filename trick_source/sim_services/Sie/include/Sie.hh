/*
    PURPOSE:
        (Sie creator)
*/

#ifndef SIE_HH
#define SIE_HH

#include <string>
#include <fstream>

#include "sim_services/Sie/include/AttributesMap.hh"
#include "sim_services/Sie/include/EnumAttributesMap.hh"

namespace Trick {

    /**
     *
     * This class wraps the MemoryManager class for use in Trick simulations
     * @author Alexander S. Lin
     *
     */
    class Sie {

        public:

            Sie() ;

            /**
             * Currently process_sim_args is an empty function
             * @return always 0
             */
            int process_sim_args() ;

            /**
             * Writes the S_sie.resource file using MemoryManager information
             * @return always 0
             */
            void sie_print_xml() ;
            void class_attr_map_print_xml() ;
            void enum_attr_map_print_xml() ;
            void top_level_objects_print_xml() ;

        private:

            void top_level_objects_print(std::ofstream & sie_out) ;

            // These are singleton maps holding all attributes known to the sim
            Trick::AttributesMap * class_attr_map ; /* ** -- This is be ignored by ICG */
            Trick::EnumAttributesMap * enum_attr_map ;   /* ** -- This is be ignored by ICG */

    } ;
}

#endif

