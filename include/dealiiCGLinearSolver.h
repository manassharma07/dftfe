// ---------------------------------------------------------------------
//
// Copyright (c) 2017-2018 The Regents of the University of Michigan and DFT-FE authors.
//
// This file is part of the DFT-FE code.
//
// The DFT-FE code is free software; you can use it, redistribute
// it, and/or modify it under the terms of the GNU Lesser General
// Public License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// The full text of the license can be found in the file LICENSE at
// the top level of the DFT-FE distribution.
//
// ---------------------------------------------------------------------
//

/**
* @brief dealii cg linear solver class wrapper
*
* @author Sambit Das
*/

#include <linearSolver.h>

#ifndef dealiiCGLinearSolver_H_
#define dealiiCGLinearSolver_H_

typedef dealii::parallel::distributed::Vector<double> vectorType;

namespace dftfe {

    class dealiiCGLinearSolver : public linearSolver
    {
       public:


	  // Constructor
	  dealiiCGLinearSolver(const  MPI_Comm &mpi_comm);

	  /**
	   * @brief Solve linear system, A*x=Rhs
	   *
	   * @param function linearSolverFunction object (functor) to compute Rhs and A*x, and preconditioning
           * @param relTolerance Tolerance (relative) required for convergence.
           * @param maxNumberIterations Maximum number of iterations.
	   * @param debugLevel Debug output level:
	   *                   0 - no debug output
	   *                   1 - limited debug output
	   *                   2 - all debug output.
	   */
	   void solve(dealiiLinearSolverFunction & function,
		      const double relTolerance,
		      const unsigned int maxNumberIterations,
		      const unsigned int  debugLevel = 0);

       private:

           MPI_Comm mpi_communicator;
           const unsigned int n_mpi_processes;
           const unsigned int this_mpi_process;
           dealii::ConditionalOStream   pcout;
    };

}

#endif
