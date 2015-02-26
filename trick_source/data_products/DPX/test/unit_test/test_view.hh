#ifndef _TEST_VIEW_H_
#define _TEST_VIEW_H_

#include <sstream>
#include "string.h"

#include "DPV/DPV_view.hh"
#include "DPC/DPC_product.hh"

class Test_view: public DPV_view {
	public: 
		Test_view() {};
		~Test_view() {};

  		DPV_pointer render_product( DPC_product* product);
  		DPV_pointer render_page( DPV_pointer parent_data, DPC_page* page);
  		DPV_pointer render_plot( DPV_pointer parent_data, DPC_plot* plot);
  		DPV_pointer render_table( DPV_pointer parent_data, DPC_table* table);
  		DPV_pointer render_curve( DPV_pointer parent_data, DPC_curve* curve);

  		void finalize_product_view( DPV_pointer product_view );
  		void finalize_page_view( DPV_pointer page_view );
  		void finalize_plot_view( DPV_pointer plot_view );
  		void finalize_table_view( DPV_pointer table_view );

		/**
		 * The following functions are unused from abstract class DPV_view
		 */
		void notify_product( DPV_pointer product_view, DPV_message msg) {};
		void notify_page( DPV_pointer page_view, DPV_message msg) {};
		void notify_table( DPV_pointer table_view, DPV_message msg) {};
		void notify_plot( DPV_pointer plot_view, DPV_message msg) {};
		void notify_curve( DPV_pointer curve_view, DPV_message msg) {};
		
		std::string getOutput();

	private:
		std::stringstream s;
};
#endif
