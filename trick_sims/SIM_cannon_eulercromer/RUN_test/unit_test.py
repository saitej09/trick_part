
execfile("Modified_data/cannon.dr")
dyn_integloop.getIntegrator(trick.Euler_Cromer, 4)
#dyn.my_integ.set_verbosity( 1)
trick.exec_set_terminate_time(5.2)
