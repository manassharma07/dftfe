//source file for dft class initializations

//initialize rho
void dftClass::initRho(){
  computing_timer.enter_section("dftClass init density"); 

  //Reading single atom rho initial guess
  pcout << "reading initial guess for rho\n";
  std::map<unsigned int, alglib::spline1dinterpolant> denSpline;
  std::map<unsigned int, std::vector<std::vector<double> > > singleAtomElectronDensity;
  std::map<unsigned int, double> outerMostPointDen;
    
  //loop over atom types
  for (std::set<unsigned int>::iterator it=atomTypes.begin(); it!=atomTypes.end(); it++){
    char densityFile[256];
    sprintf(densityFile, "../../../data/electronicStructure/z%u/density.inp", *it);
    readFile(2, singleAtomElectronDensity[*it], densityFile);
    unsigned int numRows = singleAtomElectronDensity[*it].size()-1;
    std::vector<double> xData(numRows), yData(numRows);
    for(unsigned int irow = 0; irow < numRows; ++irow){
      xData[irow] = singleAtomElectronDensity[*it][irow][0];
      yData[irow] = singleAtomElectronDensity[*it][irow][1];
    }
  
    //interpolate rho
    alglib::real_1d_array x;
    x.setcontent(numRows,&xData[0]);
    alglib::real_1d_array y;
    y.setcontent(numRows,&yData[0]);
    alglib::ae_int_t natural_bound_type = 1;
    spline1dbuildcubic(x, y, numRows, natural_bound_type, 0.0, natural_bound_type, 0.0, denSpline[*it]);
    outerMostPointDen[*it]= xData[numRows-1];
  }
  
  //Initialize rho
  QGauss<3>  quadrature_formula(FEOrder+1);
  FEValues<3> fe_values (FE, quadrature_formula, update_values);
  const unsigned int n_q_points    = quadrature_formula.size();
  //Initialize electron density table storage
  rhoInValues=new std::map<dealii::CellId, std::vector<double> >;
  rhoInVals.push_back(rhoInValues);
  //loop over elements
  typename DoFHandler<3>::active_cell_iterator cell = dofHandler.begin_active(), endc = dofHandler.end();
  for (; cell!=endc; ++cell) {
    if (cell->is_locally_owned()){
      (*rhoInValues)[cell->id()]=std::vector<double>(n_q_points);
      double *rhoInValuesPtr = &((*rhoInValues)[cell->id()][0]);
      for (unsigned int q=0; q<n_q_points; ++q){
	MappingQ<3> test(1); 
	Point<3> quadPoint(test.transform_unit_to_real_cell(cell, fe_values.get_quadrature().point(q)));
	double rhoValueAtQuadPt=0.0;
	//loop over atoms
	for (unsigned int n=0; n<atomLocations.size(); n++){
	  Point<3> atom(atomLocations[n][1],atomLocations[n][2],atomLocations[n][3]);
	  double distanceToAtom=quadPoint.distance(atom);
	  if(distanceToAtom <= outerMostPointDen[atomLocations[n][0]]){
	    rhoValueAtQuadPt+=std::abs(alglib::spline1dcalc(denSpline[atomLocations[n][0]], distanceToAtom));
	  }
	}
	rhoInValuesPtr[q]=rhoValueAtQuadPt;
      }
    }
  }
  //Normalize rho
  double charge=totalCharge();
  char buffer[100];
  sprintf(buffer, "initial total charge: %18.10e \n", charge);
  pcout << buffer;
  //scaling rho
  cell = dofHandler.begin_active();
  for (; cell!=endc; ++cell) {
    if (cell->is_locally_owned()){
      for (unsigned int q=0; q<n_q_points; ++q){
	(*rhoInValues)[cell->id()][q]*=((double)numElectrons)/charge;
      }
    }
  }
  double charge2=totalCharge();
  sprintf(buffer, "initial total charge after scaling: %18.10e \n", charge2);
  pcout << buffer;
  
  //Initialize libxc
  int exceptParamX, exceptParamC;
  exceptParamX = xc_func_init(&funcX,XC_LDA_X,XC_UNPOLARIZED);
  exceptParamC = xc_func_init(&funcC,XC_LDA_C_PZ,XC_UNPOLARIZED);
  if(exceptParamX != 0 || exceptParamC != 0){
    pcout<<"-------------------------------------"<<std::endl;
    pcout<<"Exchange or Correlation Functional not found"<<std::endl;
    pcout<<"-------------------------------------"<<std::endl;
    exit(-1);
  }
  //
  computing_timer.exit_section("dftClass init density"); 
}

void dftClass::init(){
  computing_timer.enter_section("dftClass setup");
    
  //initialize FE objects
  dofHandler.distribute_dofs (FE);
  locally_owned_dofs = dofHandler.locally_owned_dofs ();
  DoFTools::extract_locally_relevant_dofs (dofHandler, locally_relevant_dofs);
  pcout << "number of elements: "
	<< triangulation.n_global_active_cells()
	<< std::endl
	<< "number of degrees of freedom: " 
	<< dofHandler.n_dofs() 
	<< std::endl;

  //write mesh to vtk file
  DataOut<3> data_out;
  data_out.attach_dof_handler (dofHandler);
  data_out.build_patches ();
  std::ofstream output("mesh.vtu");
  data_out.write_vtu(output); 

  //matrix fee data structure
  typename MatrixFree<3>::AdditionalData additional_data;
  additional_data.mpi_communicator = MPI_COMM_WORLD;
  additional_data.tasks_parallel_scheme = MatrixFree<3>::AdditionalData::partition_partition;
  //constraints
  //hanging node constraints
  constraintsNone.clear ();
  DoFTools::make_hanging_node_constraints (dofHandler, constraintsNone);
  constraintsNone.close();
 //Zero Dirichlet BC constraints
  constraintsZero.clear ();  
  DoFTools::make_hanging_node_constraints (dofHandler, constraintsZero);
  VectorTools::interpolate_boundary_values (dofHandler, 0, ZeroFunction<3>(), constraintsZero);
  constraintsZero.close ();
  //create matrix free structure
  std::vector<const DoFHandler<3> *> dofHandlerVector; dofHandlerVector.push_back(&dofHandler); dofHandlerVector.push_back(&dofHandler);
  std::vector< const ConstraintMatrix * > constraintsVector; constraintsVector.push_back(&constraintsNone); constraintsVector.push_back(&constraintsZero);
  std::vector<Quadrature<1> > quadratureVector; quadratureVector.push_back(QGauss<1>(FEOrder+1)); quadratureVector.push_back(QGaussLobatto<1>(FEOrder+1));  
  matrix_free_data.reinit (dofHandlerVector, constraintsVector, quadratureVector, additional_data);
  //initialize eigen vectors
  matrix_free_data.initialize_dof_vector(vChebyshev);
  v0Chebyshev.reinit(vChebyshev);
  fChebyshev.reinit(vChebyshev);
  aj[0].reinit(vChebyshev); aj[1].reinit(vChebyshev); aj[2].reinit(vChebyshev);
  for (unsigned int i=0; i<eigenVectors.size(); ++i){  
    eigenVectors[i]->reinit(vChebyshev);
    PSI[i]->reinit(vChebyshev);
    tempPSI[i]->reinit(vChebyshev);
    tempPSI2[i]->reinit(vChebyshev);
    tempPSI3[i]->reinit(vChebyshev);
  } 
  
  //locate atome core nodes
  locateAtomCoreNodes();
  
  //initialize density 
  initRho();

  //
  computing_timer.exit_section("dftClass setup"); 

  //initialize poisson and eigen problem related objects
  poisson.init();
  eigen.init();
  
  //initialize PSI
  pcout << "reading initial guess for PSI\n";
  readPSI();
}