
#include <iostream>
#include <fstream>
#include <sstream>

#include "PrintFileContents10.hh"
#include "FieldDescription.hh"
#include "ClassValues.hh"
#include "EnumValues.hh"
#include "Utilities.hh"

PrintFileContents10::PrintFileContents10() {}

/** Prints the io_src header information */
void PrintFileContents10::printIOHeader(std::ofstream & outfile , std::string header_file_name) {

     if ( ! header_file_name.compare("S_source.hh") ) {
         header_file_name = "../S_source.hh" ;
     } else {
         header_file_name = almostRealPath(header_file_name.c_str()) ;
     }
     outfile << "\n"
"/*\n"
" * This file was automatically generated by the ICG based on the file:\n"
" * " << header_file_name << "\n"
" * This file contains database parameter declarations specific to the\n"
" * data structures and enumerated types declared in the above file.\n"
" * These database parameters are used by the Trick input and\n"
" * data recording processors to gain access to important simulation\n"
" * variable information.\n"
" *\n"
" * Auto Code Generator Programmer:\n"
" *    Rob Bailey  Sweet Systems Inc  12/97\n"
" *    Alex Lin    NASA               03/01\n"
" *    Alex Lin    NASA               05/09\n"
" */\n"
"#define TRICK_IN_IOSRC\n"
"#include <stdlib.h>\n"
"\n"
"#include \"trick/MemoryManager.hh\"\n"
"#include \"trick/attributes.h\"\n"
"#include \"trick/parameter_types.h\"\n"
"#include \"trick/UnitsMap.hh\"\n\n"
"#include \"trick/checkpoint_stl.hh\"\n\n"
"#include \""
<< header_file_name <<
"\"\n\n" ;

}

/** Prints enumeration attributes */
void PrintFileContents10::print_enum_attr(std::ofstream & outfile , EnumValues * e ) {
    EnumValues::NameValueIterator nvit ;

    print_open_extern_c(outfile) ;
    outfile << "ENUM_ATTR enum" ;
    printNamespaces( outfile, e , "__" ) ;
    printContainerClasses( outfile, e , "__" ) ;
    outfile << e->getName() << "[] = {\n" ;
    for ( nvit = e->begin() ; nvit != e->end() ; nvit++ ) {
        outfile << "{\"";
        printNamespaces( outfile, e , "::" ) ;
        printContainerClasses( outfile, e , "::" ) ;
        outfile << (*nvit).first << "\" , " << (*nvit).second << " , 0x0 } ,\n" ;
    }
    outfile << "{\"\" , 0 , 0x0 }\n} ;\n\n" ;
    print_close_extern_c(outfile) ;
}

/** Prints attributes for a field */
void PrintFileContents10::print_field_attr(std::ofstream & outfile ,  FieldDescription * fdes ) {
    int array_dim ;

    outfile << "{ \"" << fdes->getName() << "\"" ;        // name
    outfile << ", \"" ;                                   // start type_name
    printNamespaces( outfile, fdes , "__" ) ;
    printContainerClasses( outfile, fdes , "__" ) ;
    outfile << fdes->getMangledTypeName() << "\"";        // end type_name
    outfile << ", \"" << fdes->getUnits() << "\"" ;       // units
    outfile << ", \"\", \"\"," << std::endl ;           // alias , user_defined
    outfile << "  \"" << fdes->getDescription() << "\"," << std::endl ; // description
    outfile << "  " << fdes->getIO() ;                    // io
    outfile << "," << fdes->getEnumString() ;             // type
    // There are several cases when printing the size of a variable.
    if ( fdes->isBitField() ) {
        // bitfields are handled in 4 byte (32 bit) chunks
        outfile << ",4" ;
    } else if (  fdes->isRecord() or fdes->isEnum() or fdes->getTypeName().empty() ) {
        // records enums use io_src_get_size. The sentinel has no typename
        outfile << ",0" ;
    } else {
        // print size of the underlying type
        outfile << ",sizeof(" << fdes->getTypeName() << ")" ;
    }
    outfile << ",0,0,Language_CPP" ; // range_min, range_max, language
    outfile << "," << (fdes->isStatic() << 1 ) << "," << std::endl ;                   // mods
    if ( fdes->isBitField() ) {
        // For bitfields we need the offset to start on 4 byte boundaries because that is what our
        // insert and extract bitfield routines work with.
        outfile << "  " << (fdes->getFieldOffset() - (fdes->getFieldOffset() % 32)) / 8 ; // offset
    } else {
        outfile << "  " << (fdes->getFieldOffset() / 8) ; // offset
    }
    outfile << ",NULL" ; // attr
    outfile << "," << fdes->getNumDims() ;                // num_index

    outfile << ",{" ;
    if ( fdes->isBitField() ) {
        outfile << "{" << fdes->getBitFieldWidth() ; // size of bitfield
        outfile << "," << 32 - (fdes->getFieldOffset() % 32) - fdes->getBitFieldWidth() << "}" ; // start bit
    } else {
        array_dim = fdes->getArrayDim(0) ;
        if ( array_dim < 0 ) array_dim = 0 ;
        outfile << "{" << array_dim << ",0}" ; // index 0
    }
    unsigned int ii ;
    for ( ii = 1 ; ii < 8 ; ii++ ) {
        array_dim = fdes->getArrayDim(ii) ;
        if ( array_dim < 0 ) array_dim = 0 ;
        outfile << ",{" << array_dim << ",0}" ; // indexes 1 through 7
    }
    outfile << "}," << std::endl ;
    outfile << "  NULL, NULL, NULL, NULL" ;
    outfile << "}" ;
}

/** Prints class attributes */
void PrintFileContents10::print_class_attr(std::ofstream & outfile , ClassValues * c ) {

    unsigned int ii ;
    ClassValues::FieldIterator fit ;

    print_open_extern_c(outfile) ;
    outfile << "\nATTRIBUTES attr" ;
    printNamespaces( outfile, c , "__" ) ;
    printContainerClasses( outfile, c , "__" ) ;
    outfile << c->getMangledTypeName() ;
    outfile << "[] = {" << std::endl ;

    for ( fit = c->field_begin() ; fit != c->field_end() ; fit++ ) {
        if ( determinePrintAttr(c , *fit) ) {
            print_field_attr(outfile, *fit) ;
            outfile << "," << std::endl ;
        }
    }
    // Print an empty sentinel attribute at the end of the class.
    FieldDescription * new_fdes = new FieldDescription(std::string("")) ;
    print_field_attr(outfile, new_fdes) ;
    outfile << " };" << std::endl ;
    delete new_fdes ;

    print_close_extern_c(outfile) ;
}

/** Prints init_attr function for each class */
void PrintFileContents10::print_field_init_attr_stmts( std::ofstream & outfile , FieldDescription * fdes ,
 ClassValues * cv , unsigned int index ) {

    // For static variables replace the offset field with the address of the static variable
    if ( fdes->isStatic() ) {
        outfile << "    attr" ;
        printNamespaces( outfile, cv , "__" ) ;
        printContainerClasses( outfile, cv , "__" ) ;
        outfile << cv->getMangledTypeName() << "[" << index << "].offset = (long)(void *)&" ;
        printNamespaces( outfile, cv , "::" ) ;
        printContainerClasses( outfile, cv , "::" ) ;
        outfile << cv->getName() << "::" << fdes->getName() << " ;\n" ;
    }

    if ( fdes->isSTL()) {
        outfile << "    attr" ;
        printNamespaces( outfile, cv , "__" ) ;
        printContainerClasses( outfile, cv , "__" ) ;
        outfile << cv->getMangledTypeName() << "[" << index << "].checkpoint_stl = checkpoint_stl_" ;
        printNamespaces( outfile, cv , "__" ) ;
        printContainerClasses( outfile, cv , "__" ) ;
        outfile << cv->getMangledTypeName() ;
        outfile << "_" ;
        outfile << fdes->getName() ;
        outfile << " ;\n" ;

        outfile << "    attr" ;
        printNamespaces( outfile, cv , "__" ) ;
        printContainerClasses( outfile, cv , "__" ) ;
        outfile << cv->getMangledTypeName() << "[" << index << "].post_checkpoint_stl = post_checkpoint_stl_" ;
        printNamespaces( outfile, cv , "__" ) ;
        printContainerClasses( outfile, cv , "__" ) ;
        outfile << cv->getMangledTypeName() ;
        outfile << "_" ;
        outfile << fdes->getName() ;
        outfile << " ;\n" ;

        outfile << "    attr" ;
        printNamespaces( outfile, cv , "__" ) ;
        printContainerClasses( outfile, cv , "__" ) ;
        outfile << cv->getMangledTypeName() << "[" << index << "].restore_stl = restore_stl_" ;
        printNamespaces( outfile, cv , "__" ) ;
        printContainerClasses( outfile, cv , "__" ) ;
        outfile << cv->getMangledTypeName() ;
        outfile << "_" ;
        outfile << fdes->getName() ;
        outfile << " ;\n" ;

        if (fdes->hasSTLClear()) {
            outfile << "    attr" ;
            printNamespaces( outfile, cv , "__" ) ;
            printContainerClasses( outfile, cv , "__" ) ;
            outfile << cv->getMangledTypeName() << "[" << index << "].clear_stl = clear_stl_" ;
            printNamespaces( outfile, cv , "__" ) ;
            printContainerClasses( outfile, cv , "__" ) ;
            outfile << cv->getMangledTypeName() ;
            outfile << "_" ;
            outfile << fdes->getName() ;
            outfile << " ;\n" ;
        }
    }

    if ( fdes->isRecord() or fdes->isEnum()) {
        outfile << "    next_attr = std::string(attr" ;
        printNamespaces( outfile, cv , "__" ) ;
        printContainerClasses( outfile, cv , "__" ) ;
        outfile << cv->getMangledTypeName() << "[" << index << "].type_name) ;\n" ;

        outfile << "    mm->add_attr_info(next_attr , &attr"  ;
        printNamespaces( outfile, cv , "__" ) ;
        printContainerClasses( outfile, cv , "__" ) ;
        outfile << cv->getMangledTypeName() << "[" << index << "], __FILE__ , __LINE__ ) ;\n" ;
    }
}

/** Prints add_attr_info statements for each inherited class */
void PrintFileContents10::print_inherited_add_attr_info( std::ofstream & outfile , ClassValues * cv ) {
    ClassValues::InheritedClassesIterator cit ;
    if ( cv->getNumInheritedClasses() > 0 ) {
        outfile << "\n    ATTRIBUTES temp_attr ;\n\n" ;
    }
    for ( cit = cv->inherit_classes_begin() ; cit != cv->inherit_classes_end() ; cit++ ) {
        outfile << "    next_attr =  \"" << *cit << "\" ;\n" ;
        outfile << "    mm->add_attr_info( next_attr , &temp_attr , __FILE__ , __LINE__ ) ;\n" ;
    }
}

/** Prints init_attr function for each class */
void PrintFileContents10::print_init_attr_func( std::ofstream & outfile , ClassValues * cv ) {

    ClassValues::FieldIterator fit ;

    printOpenNamespaceBlocks(outfile, cv) ;
    outfile << "\nvoid init_attr" ;
    printNamespaces( outfile, cv , "__" ) ;
    printContainerClasses( outfile, cv , "__" ) ;
    outfile << cv->getMangledTypeName() ;
    outfile << "() {\n\n"
"    static int initialized_1337 ;\n"
"    if ( initialized_1337 ) {\n"
"            return ;\n"
"    }\n"
"    initialized_1337 = 1 ;\n\n"
"    Trick::MemoryManager * mm ;\n"
"    std::string next_attr ;\n"
"    mm = trick_MM ;\n" ;

    if ( cv->getMangledTypeName() != cv->getName() ) {
        outfile << "    typedef " << cv->getName() << " " << cv->getMangledTypeName() << " ;\n\n" ;
    }

    unsigned int ii = 0 ;
    for ( fit = cv->field_begin() ; fit != cv->field_end() ; fit++ ) {
        if ( determinePrintAttr(cv , *fit) ) {
            print_field_init_attr_stmts(outfile, *fit, cv, ii) ;
            ii++ ;
        }
    }
    print_inherited_add_attr_info(outfile, cv ) ;
    outfile << "}\n\n" ;
    printCloseNamespaceBlocks(outfile, cv) ;
}

/** Prints the io_src_sizeof function for enumerations */
void PrintFileContents10::print_enum_io_src_sizeof( std::ofstream & outfile , EnumValues * ev ) {
    print_open_extern_c(outfile) ;
    outfile << "size_t io_src_sizeof_" ;
    printNamespaces( outfile, ev , "__" ) ;
    printContainerClasses( outfile, ev , "__" ) ;
    outfile << ev->getName() << "( void ) {\n" ;
    if ( ev->getHasDefinition() ) {
        outfile << "    return( sizeof(" ;
        printNamespaces( outfile, ev , "::" ) ;
        printContainerClasses( outfile, ev , "::" ) ;
        outfile << ev->getName() << "));\n}\n\n" ;
    } else {
        outfile << "    return(sizeof(int)) ;\n}\n\n" ;
    }
    print_close_extern_c(outfile) ;
}

/** Prints the C linkage init_attr function */
void PrintFileContents10::print_init_attr_c_intf( std::ofstream & outfile , ClassValues * cv ) {
    outfile << "void init_attr" ;
    printNamespaces( outfile, cv , "__" ) ;
    printContainerClasses( outfile, cv , "__" ) ;
    outfile << cv->getMangledTypeName() << "_c_intf() {\n    " ;
    printNamespaces( outfile, cv , "::" ) ;
    outfile << "init_attr" ;
    printNamespaces( outfile, cv , "__" ) ;
    printContainerClasses( outfile, cv , "__" ) ;
    outfile << cv->getMangledTypeName() << "() ;\n"
"}\n\n" ;
}

/** Prints the io_src_sizeof function */
void PrintFileContents10::print_io_src_sizeof( std::ofstream & outfile , ClassValues * cv ) {
    outfile << "size_t io_src_sizeof_" ;
    printNamespaces( outfile, cv , "__" ) ;
    printContainerClasses( outfile, cv , "__" ) ;
    outfile << cv->getMangledTypeName() << "( void ) {\n" ;
    outfile << "    return( sizeof(" ;
    // Template types 
    if ( cv->getMangledTypeName() == cv->getName() ) {
        printNamespaces( outfile, cv , "::" ) ;
        printContainerClasses( outfile, cv , "::" ) ;
    }
    outfile << cv->getName() << "));\n"
"}\n\n" ;
}

/** Prints the io_src_allocate function */
void PrintFileContents10::print_io_src_allocate( std::ofstream & outfile , ClassValues * cv ) {
    if ( cv->isPOD() or (! cv->isAbstract() and cv->getHasDefaultConstructor()) ) {
        outfile << "void * io_src_allocate_" ;
        printNamespaces( outfile, cv , "__" ) ;
        printContainerClasses( outfile, cv , "__" ) ;
        outfile << cv->getMangledTypeName() << "( int num) {\n" ;

        outfile << "    " ;
        if ( cv->getMangledTypeName() == cv->getName() ) {
            printNamespaces( outfile, cv , "::" ) ;
            printContainerClasses( outfile, cv , "::" ) ;
        }
        outfile << cv->getName() << " * temp = (" ;
        if ( cv->getMangledTypeName() == cv->getName() ) {
            printNamespaces( outfile, cv , "::" ) ;
            printContainerClasses( outfile, cv , "::" ) ;
        }
        outfile << cv->getName() << " * )calloc( num, sizeof(" ;
        if ( cv->getMangledTypeName() == cv->getName() ) {
            printNamespaces( outfile, cv , "::" ) ;
            printContainerClasses( outfile, cv , "::" ) ;
        }
        outfile << cv->getName() << "));\n" ;
        if ( ! cv->isPOD() ) {
            outfile << "    for (int ii=0 ; ii<num ; ii++) {\n" ;
            outfile << "        new( &temp[ii]) " ;
            if ( cv->getMangledTypeName() == cv->getName() ) {
                printNamespaces( outfile, cv , "::" ) ;
                printContainerClasses( outfile, cv , "::" ) ;
            }
            outfile << cv->getName() << "();\n" << "    }\n" ;
        }
        outfile << "    return ((void *)temp);\n" << "}\n\n" ;
    }
}

/** Prints the io_src_allocate function */
void PrintFileContents10::print_io_src_destruct( std::ofstream & outfile , ClassValues * cv ) {
    if ( cv->getHasPublicDestructor()) {
        outfile << "void io_src_destruct_" ;
        printNamespaces( outfile, cv , "__" ) ;
        printContainerClasses( outfile, cv , "__" ) ;
        outfile << cv->getMangledTypeName() << "( void * addr __attribute__((unused)), int num __attribute__((unused)) ) {\n" ;
        if ( ! cv->isPOD() ) {
            // Add a using statement so we can call the destructor without fully qualifying it.
            ClassValues::NamespaceIterator nsi = cv->namespace_begin() ;
            if ( nsi != cv->namespace_end() ) {
                outfile << "    using namespace " ;
                while ( nsi != cv->namespace_end() ) {
                    outfile << *nsi ;
                    nsi++ ;
                    if ( nsi != cv->namespace_end()) {
                        outfile << "::" ;
                    }
                }
                outfile << " ;\n" ;
            }
            outfile << "    " ;
            if ( cv->getMangledTypeName() == cv->getName() ) {
                printNamespaces( outfile, cv , "::" ) ;
                printContainerClasses( outfile, cv , "::" ) ;
            }
            outfile << cv->getName() << " * temp = (" ;
            if ( cv->getMangledTypeName() == cv->getName() ) {
                printNamespaces( outfile, cv , "::" ) ;
                printContainerClasses( outfile, cv , "::" ) ;
            }
            outfile << cv->getName() << " * )addr ;\n" ;
            outfile << "    for (int ii=0 ; ii<num ; ii++) {\n" ;
            if ( cv->getMangledTypeName() == cv->getName() ) {
                outfile << "        temp[ii].~" ;
                outfile << cv->getName() << "();\n" ;
            }
            outfile << "    }\n" ;
        }
        outfile << "}\n\n" ;
    }
}

void PrintFileContents10::print_io_src_delete( std::ofstream & outfile , ClassValues * cv ) {
    if ( cv->getHasPublicDestructor()) {
        outfile << "void io_src_delete_" ;
        printNamespaces( outfile, cv , "__" ) ;
        printContainerClasses( outfile, cv , "__" ) ;
        outfile << cv->getMangledTypeName() << "( void * addr __attribute__((unused)) ) {\n" ;
        if ( ! cv->isPOD() ) {
            // Add a using statement so we can call the destructor without fully qualifying it.
            ClassValues::NamespaceIterator nsi = cv->namespace_begin() ;
            if ( nsi != cv->namespace_end() ) {
                outfile << "    using namespace " ;
                while ( nsi != cv->namespace_end() ) {
                    outfile << *nsi ;
                    nsi++ ;
                    if ( nsi != cv->namespace_end()) {
                        outfile << "::" ;
                    }
                }
                outfile << " ;\n" ;
            }
            outfile << "    " ;
            if ( cv->getMangledTypeName() == cv->getName() ) {
                printNamespaces( outfile, cv , "::" ) ;
                printContainerClasses( outfile, cv , "::" ) ;
            }
            outfile << cv->getName() << " * temp = (" ;
            if ( cv->getMangledTypeName() == cv->getName() ) {
                printNamespaces( outfile, cv , "::" ) ;
                printContainerClasses( outfile, cv , "::" ) ;
            }
            outfile << cv->getName() << " * )addr ;\n" ;
            outfile << "    delete temp ;\n" ;
        }
        outfile << "}\n\n" ;
    }
}

void PrintFileContents10::print_stl_helper_proto(std::ofstream & outfile , ClassValues * cv ) {

    unsigned int ii ;
    ClassValues::FieldIterator fit ;

    print_open_extern_c(outfile) ;

    for ( fit = cv->field_begin() ; fit != cv->field_end() ; fit++ ) {
        if ( (*fit)->isSTL() and determinePrintAttr(cv , *fit) ) {
            outfile << "void checkpoint_stl_" ;
            printNamespaces( outfile, cv , "__" ) ;
            printContainerClasses( outfile, cv , "__" ) ;
            outfile << cv->getMangledTypeName() ;
            outfile << "_" ;
            outfile << (*fit)->getName() ;
            outfile << "(void * start_address, const char * obj_name , const char * var_name) ;" << std::endl ;

            outfile << "void post_checkpoint_stl_" ;
            printNamespaces( outfile, cv , "__" ) ;
            printContainerClasses( outfile, cv , "__" ) ;
            outfile << cv->getMangledTypeName() ;
            outfile << "_" ;
            outfile << (*fit)->getName() ;
            outfile << "(void * start_address, const char * obj_name , const char * var_name) ;" << std::endl ;

            outfile << "void restore_stl_" ;
            printNamespaces( outfile, cv , "__" ) ;
            printContainerClasses( outfile, cv , "__" ) ;
            outfile << cv->getMangledTypeName() ;
            outfile << "_" ;
            outfile << (*fit)->getName() ;
            outfile << "(void * start_address, const char * obj_name , const char * var_name) ;" << std::endl ;

            if ((*fit)->hasSTLClear()) {
                outfile << "void clear_stl_" ;
                printNamespaces( outfile, cv , "__" ) ;
                printContainerClasses( outfile, cv , "__" ) ;
                outfile << cv->getMangledTypeName() ;
                outfile << "_" ;
                outfile << (*fit)->getName() ;
                outfile << "(void * start_address) ;" << std::endl ;
            }
        }
    }
    print_close_extern_c(outfile) ;
}

void PrintFileContents10::print_checkpoint_stl(std::ofstream & outfile , FieldDescription * fdes , ClassValues * cv ) {
    outfile << "void checkpoint_stl_" ;
    printNamespaces( outfile, cv , "__" ) ;
    printContainerClasses( outfile, cv , "__" ) ;
    outfile << cv->getMangledTypeName() ;
    outfile << "_" ;
    outfile << fdes->getName() ;
    outfile << "(void * start_address, const char * obj_name , const char * var_name) {" << std::endl ;

    outfile << "    " << fdes->getTypeName() << " * stl = reinterpret_cast<" << fdes->getTypeName() << " * >(start_address) ;" << std::endl ;
    outfile << "    " << "checkpoint_stl(*stl , obj_name , var_name) ;" << std::endl ;

    outfile << "}" << std::endl ;
}

void PrintFileContents10::print_post_checkpoint_stl(std::ofstream & outfile , FieldDescription * fdes , ClassValues * cv ) {
    outfile << "void post_checkpoint_stl_" ;
    printNamespaces( outfile, cv , "__" ) ;
    printContainerClasses( outfile, cv , "__" ) ;
    outfile << cv->getMangledTypeName() ;
    outfile << "_" ;
    outfile << fdes->getName() ;
    outfile << "(void * start_address, const char * obj_name , const char * var_name) {" << std::endl ;

    outfile << "    " << fdes->getTypeName() << " * stl = reinterpret_cast<" << fdes->getTypeName() << " * >(start_address) ;" << std::endl ;
    outfile << "    " << "delete_stl(*stl , obj_name , var_name) ;" << std::endl ;

    outfile << "}" << std::endl ;
}

void PrintFileContents10::print_restore_stl(std::ofstream & outfile , FieldDescription * fdes , ClassValues * cv ) {
    outfile << "void restore_stl_" ;
    printNamespaces( outfile, cv , "__" ) ;
    printContainerClasses( outfile, cv , "__" ) ;
    outfile << cv->getMangledTypeName() ;
    outfile << "_" ;
    outfile << fdes->getName() ;
    outfile << "(void * start_address, const char * obj_name , const char * var_name) {" << std::endl ;

    outfile << "    " << fdes->getTypeName() << " * stl = reinterpret_cast<" << fdes->getTypeName() << " * >(start_address) ;" << std::endl ;
    outfile << "    " << "restore_stl(*stl , obj_name , var_name) ;" << std::endl ;

    outfile << "}" << std::endl ;
}

void PrintFileContents10::print_clear_stl(std::ofstream & outfile , FieldDescription * fdes , ClassValues * cv ) {
    outfile << "void clear_stl_" ;
    printNamespaces( outfile, cv , "__" ) ;
    printContainerClasses( outfile, cv , "__" ) ;
    outfile << cv->getMangledTypeName() ;
    outfile << "_" ;
    outfile << fdes->getName() ;
    outfile << "(void * start_address) {" << std::endl ;

    outfile << "    " << fdes->getTypeName() << " * stl = reinterpret_cast<" << fdes->getTypeName() << " * >(start_address) ;" << std::endl ;
    outfile << "    " << "stl->clear() ;" << std::endl ;

    outfile << "}" << std::endl ;
}

void PrintFileContents10::print_stl_helper(std::ofstream & outfile , ClassValues * cv ) {

    unsigned int ii ;
    ClassValues::FieldIterator fit ;

    print_open_extern_c(outfile) ;

    for ( fit = cv->field_begin() ; fit != cv->field_end() ; fit++ ) {
        if ( (*fit)->isSTL() and determinePrintAttr(cv , *fit) ) {
            print_checkpoint_stl(outfile , *fit, cv) ;
            print_post_checkpoint_stl(outfile , *fit, cv) ;
            print_restore_stl(outfile , *fit, cv) ;
            if ((*fit)->hasSTLClear()) {
                print_clear_stl(outfile , *fit, cv) ;
            }
        }
    }
    print_close_extern_c(outfile) ;
}

void PrintFileContents10::printClass( std::ofstream & outfile , ClassValues * cv ) {
    print_stl_helper_proto(outfile, cv) ;
    print_class_attr(outfile, cv) ;
    print_stl_helper(outfile, cv) ;
    print_init_attr_func(outfile, cv) ;
    print_open_extern_c(outfile) ;
    print_init_attr_c_intf(outfile, cv) ;
    print_io_src_sizeof(outfile, cv) ;
    print_io_src_allocate(outfile, cv) ;
    print_io_src_destruct(outfile, cv) ;
    print_io_src_delete(outfile, cv) ;
    print_close_extern_c(outfile) ;
    print_units_map(outfile, cv) ;
}

void PrintFileContents10::printEnum( std::ofstream & outfile , EnumValues * ev ) {
    print_enum_attr(outfile, ev) ;
    print_enum_io_src_sizeof(outfile, ev) ;
}

void PrintFileContents10::printClassMapHeader( std::ofstream & outfile , std::string function_name ) {
     outfile <<
"/*\n"
" * This file was automatically generated by the ICG\n"
" * This file contains the map from class/struct names to attributes\n"
" */\n\n"
"#include <map>\n"
"#include <string>\n\n"
"#include \"trick/AttributesMap.hh\"\n"
"#include \"trick/EnumAttributesMap.hh\"\n"
"#include \"trick/attributes.h\"\n\n"
"void " << function_name << "() {\n\n"
"    Trick::AttributesMap * class_attribute_map = Trick::AttributesMap::attributes_map();\n\n" ;
}

void PrintFileContents10::printClassMap( std::ofstream & outfile , ClassValues * cv ) {
    outfile << "    // " << cv->getFileName() << std::endl ;
    outfile << "    extern ATTRIBUTES  attr" ;
    printNamespaces( outfile, cv , "__" ) ;
    printContainerClasses( outfile, cv , "__" ) ;
    outfile << cv->getMangledTypeName() << "[] ;" << std::endl ;

    outfile << "    class_attribute_map->add_attr(\"" ;
    printNamespaces( outfile, cv , "::" ) ;
    printContainerClasses( outfile, cv , "::" ) ;
    outfile << cv->getMangledTypeName() << "\" , attr" ;
    printNamespaces( outfile, cv , "__" ) ;
    printContainerClasses( outfile, cv , "__" ) ;
    outfile << cv->getMangledTypeName() << ") ;" << std::endl ;
}

void PrintFileContents10::printClassMapFooter( std::ofstream & outfile ) {
     outfile << "}" << std::endl << std::endl ;
}

void PrintFileContents10::printEnumMapHeader( std::ofstream & outfile , std::string function_name ) {
     outfile <<
"void " << function_name << "() {\n"
"    Trick::EnumAttributesMap * enum_attribute_map __attribute__((unused)) = Trick::EnumAttributesMap::attributes_map();\n\n" ;
}

void PrintFileContents10::printEnumMap( std::ofstream & outfile , EnumValues * ev ) {
    outfile << "    extern ENUM_ATTR  enum" ;
    printNamespaces( outfile, ev , "__" ) ;
    printContainerClasses( outfile, ev , "__" ) ;
    outfile << ev->getName() << "[] ;" << std::endl ;

    outfile << "    enum_attribute_map->add_attr(\"" ;
    printNamespaces( outfile, ev , "::" ) ;
    printContainerClasses( outfile, ev , "::" ) ;
    outfile << ev->getName() << "\" , enum" ;
    printNamespaces( outfile, ev , "__" ) ;
    printContainerClasses( outfile, ev , "__" ) ;
    outfile << ev->getName() << ") ;" << std::endl ;
}

void PrintFileContents10::printEnumMapFooter( std::ofstream & outfile ) {
     outfile << "}" << std::endl << std::endl ;
}
