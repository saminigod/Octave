/*

Copyright (C) 1996, 1997 John W. Eaton

This file is part of Octave.

Octave is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

Octave is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Octave; see the file COPYING.  If not, write to the Free
Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cassert>
#include <climits>

#include "Array-util.h"
#include "CColVector.h"
#include "CDiagMatrix.h"
#include "CMatrix.h"
#include "EIG.h"
#include "dDiagMatrix.h"
#include "dMatrix.h"
#include "mx-cm-cdm.h"
#include "oct-cmplx.h"
#include "quit.h"

#include "error.h"
#include "oct-obj.h"
#include "utils.h"
#include "xpow.h"

#ifdef _OPENMP
#include <omp.h>
#endif

static inline int
xisint (double x)
{
  return (D_NINT (x) == x
	  && ((x >= 0 && x < INT_MAX)
	      || (x <= 0 && x > INT_MIN)));
}

// Safer pow functions.
//
//       op2 \ op1:   s   m   cs   cm
//            +--   +---+---+----+----+
//   scalar   |     | 1 | 5 |  7 | 11 |
//                  +---+---+----+----+
//   matrix         | 2 | * |  8 |  * |
//                  +---+---+----+----+
//   complex_scalar | 3 | 6 |  9 | 12 |
//                  +---+---+----+----+
//   complex_matrix | 4 | * | 10 |  * |
//                  +---+---+----+----+

// -*- 1 -*-
octave_value
xpow (double a, double b)
{
  if (a < 0.0 && static_cast<int> (b) != b)
    {
      Complex atmp (a);

      return std::pow (atmp, b);
    }
  else
    return std::pow (a, b);
}

// -*- 2 -*-
octave_value
xpow (double a, const Matrix& b)
{
  octave_value retval;

  octave_idx_type nr = b.rows ();
  octave_idx_type nc = b.cols ();

  if (nr == 0 || nc == 0 || nr != nc)
    error ("for x^A, A must be square");
  else
    {
      EIG b_eig (b);
      ComplexColumnVector lambda (b_eig.eigenvalues ());
      ComplexMatrix Q (b_eig.eigenvectors ());

      for (octave_idx_type i = 0; i < nr; i++)
	{
	  Complex elt = lambda (i);
	  if (std::imag (elt) == 0.0)
	    lambda (i) = std::pow (a, std::real (elt));
	  else
	    lambda (i) = std::pow (a, elt);
	}
      ComplexDiagMatrix D (lambda);

      retval = ComplexMatrix (Q * D * Q.inverse ());
    }

  return retval;
}

// -*- 3 -*-
octave_value
xpow (double a, const Complex& b)
{
  Complex result;
  Complex atmp (a);
  result = std::pow (atmp, b);
  return result;
}

// -*- 4 -*-
octave_value
xpow (double a, const ComplexMatrix& b)
{
  octave_value retval;

  octave_idx_type nr = b.rows ();
  octave_idx_type nc = b.cols ();

  if (nr == 0 || nc == 0 || nr != nc)
    error ("for x^A, A must be square");
  else
    {
      EIG b_eig (b);
      ComplexColumnVector lambda (b_eig.eigenvalues ());
      ComplexMatrix Q (b_eig.eigenvectors ());

      for (octave_idx_type i = 0; i < nr; i++)
	{
	  Complex elt = lambda (i);
	  if (std::imag (elt) == 0.0)
	    lambda (i) = std::pow (a, std::real (elt));
	  else
	    lambda (i) = std::pow (a, elt);
	}
      ComplexDiagMatrix D (lambda);

      retval = ComplexMatrix (Q * D * Q.inverse ());
    }

  return retval;
}

// -*- 5 -*-
octave_value
xpow (const Matrix& a, double b)
{
  octave_value retval;

  octave_idx_type nr = a.rows ();
  octave_idx_type nc = a.cols ();

  if (nr == 0 || nc == 0 || nr != nc)
    error ("for A^b, A must be square");
  else
    {
      if (static_cast<int> (b) == b)
	{
	  int btmp = static_cast<int> (b);
	  if (btmp == 0)
	    {
	      retval = DiagMatrix (nr, nr, 1.0);
	    }
	  else
	    {
	      // Too much copying?
	      // XXX FIXME XXX -- we shouldn't do this if the exponent is
	      // large...

	      Matrix atmp;
	      if (btmp < 0)
		{
		  btmp = -btmp;

		  octave_idx_type info;
		  double rcond = 0.0;

		  atmp = a.inverse (info, rcond, 1);

		  if (info == -1)
		    warning ("inverse: matrix singular to machine\
 precision, rcond = %g", rcond);
		}
	      else
		atmp = a;

	      Matrix result (atmp);

	      btmp--;

	      while (btmp > 0)
		{
		  if (btmp & 1)
		    result = result * atmp;

		  btmp >>= 1;

		  if (btmp > 0)
		    atmp = atmp * atmp;
		}

	      retval = result;
	    }
	}
      else
	{
	  EIG a_eig (a);
	  ComplexColumnVector lambda (a_eig.eigenvalues ());
	  ComplexMatrix Q (a_eig.eigenvectors ());

	  for (octave_idx_type i = 0; i < nr; i++)
	    lambda (i) = std::pow (lambda (i), b);

	  ComplexDiagMatrix D (lambda);

	  retval = ComplexMatrix (Q * D * Q.inverse ());
	}
    }

  return retval;
}

// -*- 6 -*-
octave_value
xpow (const Matrix& a, const Complex& b)
{
  octave_value retval;

  octave_idx_type nr = a.rows ();
  octave_idx_type nc = a.cols ();

  if (nr == 0 || nc == 0 || nr != nc)
    error ("for A^b, A must be square");
  else
    {
      EIG a_eig (a);
      ComplexColumnVector lambda (a_eig.eigenvalues ());
      ComplexMatrix Q (a_eig.eigenvectors ());

      for (octave_idx_type i = 0; i < nr; i++)
	lambda (i) = std::pow (lambda (i), b);

      ComplexDiagMatrix D (lambda);

      retval = ComplexMatrix (Q * D * Q.inverse ());
    }

  return retval;
}

// -*- 7 -*-
octave_value
xpow (const Complex& a, double b)
{
  Complex result;

  if (xisint (b))
    result = std::pow (a, static_cast<int> (b));
  else
    result = std::pow (a, b);

  return result;
}

// -*- 8 -*-
octave_value
xpow (const Complex& a, const Matrix& b)
{
  octave_value retval;

  octave_idx_type nr = b.rows ();
  octave_idx_type nc = b.cols ();

  if (nr == 0 || nc == 0 || nr != nc)
    error ("for x^A, A must be square");
  else
    {
      EIG b_eig (b);
      ComplexColumnVector lambda (b_eig.eigenvalues ());
      ComplexMatrix Q (b_eig.eigenvectors ());

      for (octave_idx_type i = 0; i < nr; i++)
	{
	  Complex elt = lambda (i);
	  if (std::imag (elt) == 0.0)
	    lambda (i) = std::pow (a, std::real (elt));
	  else
	    lambda (i) = std::pow (a, elt);
	}
      ComplexDiagMatrix D (lambda);

      retval = ComplexMatrix (Q * D * Q.inverse ());
    }

  return retval;
}

// -*- 9 -*-
octave_value
xpow (const Complex& a, const Complex& b)
{
  Complex result;
  result = std::pow (a, b);
  return result;
}

// -*- 10 -*-
octave_value
xpow (const Complex& a, const ComplexMatrix& b)
{
  octave_value retval;

  octave_idx_type nr = b.rows ();
  octave_idx_type nc = b.cols ();

  if (nr == 0 || nc == 0 || nr != nc)
    error ("for x^A, A must be square");
  else
    {
      EIG b_eig (b);
      ComplexColumnVector lambda (b_eig.eigenvalues ());
      ComplexMatrix Q (b_eig.eigenvectors ());

      for (octave_idx_type i = 0; i < nr; i++)
	{
	  Complex elt = lambda (i);
	  if (std::imag (elt) == 0.0)
	    lambda (i) = std::pow (a, std::real (elt));
	  else
	    lambda (i) = std::pow (a, elt);
	}
      ComplexDiagMatrix D (lambda);

      retval = ComplexMatrix (Q * D * Q.inverse ());
    }

  return retval;
}

// -*- 11 -*-
octave_value
xpow (const ComplexMatrix& a, double b)
{
  octave_value retval;

  octave_idx_type nr = a.rows ();
  octave_idx_type nc = a.cols ();

  if (nr == 0 || nc == 0 || nr != nc)
    error ("for A^b, A must be square");
  else
    {
      if (static_cast<int> (b) == b)
	{
	  int btmp = static_cast<int> (b);
	  if (btmp == 0)
	    {
	      retval = DiagMatrix (nr, nr, 1.0);
	    }
	  else
	    {
	      // Too much copying?
	      // XXX FIXME XXX -- we shouldn't do this if the exponent is
	      // large...

	      ComplexMatrix atmp;
	      if (btmp < 0)
		{
		  btmp = -btmp;

		  octave_idx_type info;
		  double rcond = 0.0;

		  atmp = a.inverse (info, rcond, 1);

		  if (info == -1)
		    warning ("inverse: matrix singular to machine\
 precision, rcond = %g", rcond);
		}
	      else
		atmp = a;

	      ComplexMatrix result (atmp);

	      btmp--;

	      while (btmp > 0)
		{
		  if (btmp & 1)
		    result = result * atmp;

		  btmp >>= 1;

		  if (btmp > 0)
		    atmp = atmp * atmp;
		}

	      retval = result;
	    }
	}
      else
	{
	  EIG a_eig (a);
	  ComplexColumnVector lambda (a_eig.eigenvalues ());
	  ComplexMatrix Q (a_eig.eigenvectors ());

	  for (octave_idx_type i = 0; i < nr; i++)
	    lambda (i) = std::pow (lambda (i), b);

	  ComplexDiagMatrix D (lambda);

	  retval = ComplexMatrix (Q * D * Q.inverse ());
	}
    }

  return retval;
}

// -*- 12 -*-
octave_value
xpow (const ComplexMatrix& a, const Complex& b)
{
  octave_value retval;

  octave_idx_type nr = a.rows ();
  octave_idx_type nc = a.cols ();

  if (nr == 0 || nc == 0 || nr != nc)
    error ("for A^b, A must be square");
  else
    {
      EIG a_eig (a);
      ComplexColumnVector lambda (a_eig.eigenvalues ());
      ComplexMatrix Q (a_eig.eigenvectors ());

      for (octave_idx_type i = 0; i < nr; i++)
	lambda (i) = std::pow (lambda (i), b);

      ComplexDiagMatrix D (lambda);

      retval = ComplexMatrix (Q * D * Q.inverse ());
    }

  return retval;
}

// Safer pow functions that work elementwise for matrices.
//
//       op2 \ op1:   s   m   cs   cm
//            +--   +---+---+----+----+
//   scalar   |     | * | 3 |  * |  9 |
//                  +---+---+----+----+
//   matrix         | 1 | 4 |  7 | 10 |
//                  +---+---+----+----+
//   complex_scalar | * | 5 |  * | 11 |
//                  +---+---+----+----+
//   complex_matrix | 2 | 6 |  8 | 12 |
//                  +---+---+----+----+
//
//   * -> not needed.

// XXX FIXME XXX -- these functions need to be fixed so that things
// like
//
//   a = -1; b = [ 0, 0.5, 1 ]; r = a .^ b
//
// and
//
//   a = -1; b = [ 0, 0.5, 1 ]; for i = 1:3, r(i) = a .^ b(i), end
//
// produce identical results.  Also, it would be nice if -1^0.5
// produced a pure imaginary result instead of a complex number with a
// small real part.  But perhaps that's really a problem with the math
// library...

// -*- 1 -*-
octave_value
elem_xpow (double a, const Matrix& b)
{
  octave_value retval;

  octave_idx_type nr = b.rows ();
  octave_idx_type nc = b.cols ();

  double d1, d2;

  if (a < 0.0 && ! b.all_integers (d1, d2))
    {
      Complex atmp (a);
      ComplexMatrix result (nr, nc);

      for (octave_idx_type j = 0; j < nc; j++)
	for (octave_idx_type i = 0; i < nr; i++)
	  {
	    OCTAVE_QUIT;
	    result (i, j) = std::pow (atmp, b (i, j));
	  }

      retval = result;
    }
  else
    {
      Matrix result (nr, nc);

      for (octave_idx_type j = 0; j < nc; j++)
	for (octave_idx_type i = 0; i < nr; i++)
	  {
	    OCTAVE_QUIT;
	    result (i, j) = std::pow (a, b (i, j));
	  }

      retval = result;
    }

  return retval;
}

// -*- 2 -*-
octave_value
elem_xpow (double a, const ComplexMatrix& b)
{
  octave_idx_type nr = b.rows ();
  octave_idx_type nc = b.cols ();

  ComplexMatrix result (nr, nc);
  Complex atmp (a);

  for (octave_idx_type j = 0; j < nc; j++)
    for (octave_idx_type i = 0; i < nr; i++)
      {
	OCTAVE_QUIT;
	result (i, j) = std::pow (atmp, b (i, j));
      }

  return result;
}

// -*- 3 -*-
octave_value
elem_xpow (const Matrix& a, double b)
{
  octave_value retval;

  octave_idx_type nr = a.rows ();
  octave_idx_type nc = a.cols ();

  if (static_cast<int> (b) != b && a.any_element_is_negative ())
    {
      ComplexMatrix result (nr, nc);

      for (octave_idx_type j = 0; j < nc; j++)
	for (octave_idx_type i = 0; i < nr; i++)
	  {
	    OCTAVE_QUIT; 
      
	    Complex atmp (a (i, j));

	    result (i, j) = std::pow (atmp, b);
	  }

      retval = result;
    }
  else
    {
      Matrix result (nr, nc);

      for (octave_idx_type j = 0; j < nc; j++)
	for (octave_idx_type i = 0; i < nr; i++)
	  {
	    OCTAVE_QUIT;
	    result (i, j) = std::pow (a (i, j), b);
	  }

      retval = result;
    }

  return retval;
}

// -*- 4 -*-
octave_value
elem_xpow (const Matrix& a, const Matrix& b)
{
  octave_value retval;

  octave_idx_type nr = a.rows ();
  octave_idx_type nc = a.cols ();

  octave_idx_type b_nr = b.rows ();
  octave_idx_type b_nc = b.cols ();

  if (nr != b_nr || nc != b_nc)
    {
      gripe_nonconformant ("operator .^", nr, nc, b_nr, b_nc);
      return octave_value ();
    }

  int convert_to_complex = 0;
  for (octave_idx_type j = 0; j < nc; j++)
    for (octave_idx_type i = 0; i < nr; i++)
      {
	OCTAVE_QUIT;
	double atmp = a (i, j);
	double btmp = b (i, j);
	if (atmp < 0.0 && static_cast<int> (btmp) != btmp)
	  {
	    convert_to_complex = 1;
	    goto done;
	  }
      }

done:

  if (convert_to_complex)
    {
      ComplexMatrix complex_result (nr, nc);

      for (octave_idx_type j = 0; j < nc; j++)
	for (octave_idx_type i = 0; i < nr; i++)
	  {
	    OCTAVE_QUIT;
	    Complex atmp (a (i, j));
	    Complex btmp (b (i, j));
	    complex_result (i, j) = std::pow (atmp, btmp);
	  }

      retval = complex_result;
    }
  else
    {
      Matrix result (nr, nc);

      for (octave_idx_type j = 0; j < nc; j++)
	for (octave_idx_type i = 0; i < nr; i++)
	  {
	    OCTAVE_QUIT;
	    result (i, j) = std::pow (a (i, j), b (i, j));
	  }

      retval = result;
    }

  return retval;
}

// -*- 5 -*-
octave_value
elem_xpow (const Matrix& a, const Complex& b)
{
  octave_idx_type nr = a.rows ();
  octave_idx_type nc = a.cols ();

  ComplexMatrix result (nr, nc);

  for (octave_idx_type j = 0; j < nc; j++)
    for (octave_idx_type i = 0; i < nr; i++)
      {
	OCTAVE_QUIT;
	result (i, j) = std::pow (Complex (a (i, j)), b);
      }

  return result;
}

// -*- 6 -*-
octave_value
elem_xpow (const Matrix& a, const ComplexMatrix& b)
{
  octave_idx_type nr = a.rows ();
  octave_idx_type nc = a.cols ();

  octave_idx_type b_nr = b.rows ();
  octave_idx_type b_nc = b.cols ();

  if (nr != b_nr || nc != b_nc)
    {
      gripe_nonconformant ("operator .^", nr, nc, b_nr, b_nc);
      return octave_value ();
    }

  ComplexMatrix result (nr, nc);

  for (octave_idx_type j = 0; j < nc; j++)
    for (octave_idx_type i = 0; i < nr; i++)
      {
	OCTAVE_QUIT;
	result (i, j) = std::pow (Complex (a (i, j)), b (i, j));
      }

  return result;
}

// -*- 7 -*-
octave_value
elem_xpow (const Complex& a, const Matrix& b)
{
  octave_idx_type nr = b.rows ();
  octave_idx_type nc = b.cols ();

  ComplexMatrix result (nr, nc);

  for (octave_idx_type j = 0; j < nc; j++)
    for (octave_idx_type i = 0; i < nr; i++)
      {
	OCTAVE_QUIT;
	double btmp = b (i, j);
	if (xisint (btmp))
	  result (i, j) = std::pow (a, static_cast<int> (btmp));
	else
	  result (i, j) = std::pow (a, btmp);
      }

  return result;
}

// -*- 8 -*-
octave_value
elem_xpow (const Complex& a, const ComplexMatrix& b)
{
  octave_idx_type nr = b.rows ();
  octave_idx_type nc = b.cols ();

  ComplexMatrix result (nr, nc);

  for (octave_idx_type j = 0; j < nc; j++)
    for (octave_idx_type i = 0; i < nr; i++)
      {
	OCTAVE_QUIT;
	result (i, j) = std::pow (a, b (i, j));
      }

  return result;
}

// -*- 9 -*-
octave_value
elem_xpow (const ComplexMatrix& a, double b)
{
  octave_idx_type nr = a.rows ();
  octave_idx_type nc = a.cols ();

  ComplexMatrix result (nr, nc);

  if (xisint (b))
    {
      for (octave_idx_type j = 0; j < nc; j++)
	for (octave_idx_type i = 0; i < nr; i++)
	  {
	    OCTAVE_QUIT;
	    result (i, j) = std::pow (a (i, j), static_cast<int> (b));
	  }
    }
  else
    {
      for (octave_idx_type j = 0; j < nc; j++)
	for (octave_idx_type i = 0; i < nr; i++)
	  {
	    OCTAVE_QUIT;
	    result (i, j) = std::pow (a (i, j), b);
	  }
    }

  return result;
}

// -*- 10 -*-
octave_value
elem_xpow (const ComplexMatrix& a, const Matrix& b)
{
  octave_idx_type nr = a.rows ();
  octave_idx_type nc = a.cols ();

  octave_idx_type b_nr = b.rows ();
  octave_idx_type b_nc = b.cols ();

  if (nr != b_nr || nc != b_nc)
    {
      gripe_nonconformant ("operator .^", nr, nc, b_nr, b_nc);
      return octave_value ();
    }

  ComplexMatrix result (nr, nc);

  for (octave_idx_type j = 0; j < nc; j++)
    for (octave_idx_type i = 0; i < nr; i++)
      {
	OCTAVE_QUIT;
	double btmp = b (i, j);
	if (xisint (btmp))
	  result (i, j) = std::pow (a (i, j), static_cast<int> (btmp));
	else
	  result (i, j) = std::pow (a (i, j), btmp);
      }

  return result;
}

// -*- 11 -*-
octave_value
elem_xpow (const ComplexMatrix& a, const Complex& b)
{
  octave_idx_type nr = a.rows ();
  octave_idx_type nc = a.cols ();

  ComplexMatrix result (nr, nc);

  for (octave_idx_type j = 0; j < nc; j++)
    for (octave_idx_type i = 0; i < nr; i++)
      {
	OCTAVE_QUIT;
	result (i, j) = std::pow (a (i, j), b);
      }

  return result;
}

// -*- 12 -*-
octave_value
elem_xpow (const ComplexMatrix& a, const ComplexMatrix& b)
{
  octave_idx_type nr = a.rows ();
  octave_idx_type nc = a.cols ();

  octave_idx_type b_nr = b.rows ();
  octave_idx_type b_nc = b.cols ();

  if (nr != b_nr || nc != b_nc)
    {
      gripe_nonconformant ("operator .^", nr, nc, b_nr, b_nc);
      return octave_value ();
    }

  ComplexMatrix result (nr, nc);

  for (octave_idx_type j = 0; j < nc; j++)
    for (octave_idx_type i = 0; i < nr; i++)
      {
	OCTAVE_QUIT;
	result (i, j) = std::pow (a (i, j), b (i, j));
      }

  return result;
}

// Safer pow functions that work elementwise for N-d arrays.
//
//       op2 \ op1:   s   nd  cs   cnd
//            +--   +---+---+----+----+
//   scalar   |     | * | 3 |  * |  9 |
//                  +---+---+----+----+
//   N_d            | 1 | 4 |  7 | 10 |
//                  +---+---+----+----+
//   complex_scalar | * | 5 |  * | 11 |
//                  +---+---+----+----+
//   complex_N_d    | 2 | 6 |  8 | 12 |
//                  +---+---+----+----+
//
//   * -> not needed.

// XXX FIXME XXX -- these functions need to be fixed so that things
// like
//
//   a = -1; b = [ 0, 0.5, 1 ]; r = a .^ b
//
// and
//
//   a = -1; b = [ 0, 0.5, 1 ]; for i = 1:3, r(i) = a .^ b(i), end
//
// produce identical results.  Also, it would be nice if -1^0.5
// produced a pure imaginary result instead of a complex number with a
// small real part.  But perhaps that's really a problem with the math
// library...

// -*- 1 -*-
octave_value
elem_xpow (double a, const NDArray& b)
{
  octave_value retval;

  double d1, d2;

  if (a < 0.0 && ! b.all_integers (d1, d2))
    {
      Complex atmp (a);
      ComplexNDArray result (b.dims ());
      for (octave_idx_type i = 0; i < b.length (); i++)
	{
	  OCTAVE_QUIT;
	  result(i) = std::pow (atmp, b(i));
	}

      retval = result;
    }
  else
    {
      NDArray result (b.dims ());
      for (octave_idx_type i = 0; i < b.length (); i++)
	{
	  OCTAVE_QUIT;
	  result (i) = std::pow (a, b(i));
	}

      retval = result;
    }

  return retval;
}

// -*- 2 -*-
octave_value
elem_xpow (double a, const ComplexNDArray& b)
{
  ComplexNDArray result (b.dims ());
  Complex atmp (a);

  for (octave_idx_type i = 0; i < b.length (); i++)
    {
      OCTAVE_QUIT;
      result(i) = std::pow (atmp, b(i));
    }

  return result;
}

// -*- 3 -*-
octave_value
elem_xpow (const NDArray& a, double b)
{
  octave_value retval;

  if (static_cast<int> (b) != b && a.any_element_is_negative ())
    {
      ComplexNDArray result (a.dims ());

      for (octave_idx_type i = 0; i < a.length (); i++)
	{
	  OCTAVE_QUIT;

	  Complex atmp (a (i));

	  result(i) = std::pow (atmp, b);
	}

      retval = result;
    }
  else
    {
      NDArray result (a.dims ());

      for (octave_idx_type i = 0; i < a.length (); i++)
	{
	  OCTAVE_QUIT;
	  result(i) = std::pow (a(i), b);
	}

      retval = result;
    }

  return retval;
}

// -*- 4 -*-
octave_value
elem_xpow (const NDArray& a, const NDArray& b)
{
  octave_value retval;

  dim_vector a_dims = a.dims ();
  dim_vector b_dims = b.dims ();

  if (a_dims != b_dims)
    {
      gripe_nonconformant ("operator .^", a_dims, b_dims);
      return octave_value ();
    }

  int len = a.length ();

  bool convert_to_complex = false;

  for (octave_idx_type i = 0; i < len; i++)
    {
      OCTAVE_QUIT;
      double atmp = a(i);
      double btmp = b(i);
      if (atmp < 0.0 && static_cast<int> (btmp) != btmp)
	{
	  convert_to_complex = true;
	  goto done;
	}
    }

done:

  if (convert_to_complex)
    {
      ComplexNDArray complex_result (a_dims);

      for (octave_idx_type i = 0; i < len; i++)
	{
	  OCTAVE_QUIT;
	  Complex atmp (a(i));
	  Complex btmp (b(i));
	  complex_result(i) = std::pow (atmp, btmp);
	}

      retval = complex_result;
    }
  else
    {
      NDArray result (a_dims);

      for (octave_idx_type i = 0; i < len; i++)
	{
	  OCTAVE_QUIT;
	  result(i) = std::pow (a(i), b(i));
	}

      retval = result;
    }

  return retval;
}

// -*- 5 -*-
octave_value
elem_xpow (const NDArray& a, const Complex& b)
{
  ComplexNDArray result (a.dims ());

  for (octave_idx_type i = 0; i < a.length (); i++)
    {
      OCTAVE_QUIT;
      result(i) = std::pow (Complex (a(i)), b);
    }

  return result;
}

// -*- 6 -*-
octave_value
elem_xpow (const NDArray& a, const ComplexNDArray& b)
{
  dim_vector a_dims = a.dims ();
  dim_vector b_dims = b.dims ();

  if (a_dims != b_dims)
    {
      gripe_nonconformant ("operator .^", a_dims, b_dims);
      return octave_value ();
    }

  ComplexNDArray result (a_dims);

  for (octave_idx_type i = 0; i < a.length (); i++)
    {
      OCTAVE_QUIT;
      result(i) = std::pow (Complex (a(i)), b(i));
    }

  return result;
}

// -*- 7 -*-
octave_value
elem_xpow (const Complex& a, const NDArray& b)
{
  ComplexNDArray result (b.dims ());

  for (octave_idx_type i = 0; i < b.length (); i++)
    {
      OCTAVE_QUIT;
      double btmp = b(i);
      if (xisint (btmp))
	result(i) = std::pow (a, static_cast<int> (btmp));
      else
	result(i) = std::pow (a, btmp);
    }

  return result;
}

// -*- 8 -*-
octave_value
elem_xpow (const Complex& a, const ComplexNDArray& b)
{
  ComplexNDArray result (b.dims ());

  for (octave_idx_type i = 0; i < b.length (); i++)
    {
      OCTAVE_QUIT;
      result(i) = std::pow (a, b(i));
    }

  return result;
}

// -*- 9 -*-
octave_value
elem_xpow (const ComplexNDArray& a, double b)
{
  ComplexNDArray result (a.dims ());

  if (xisint (b))
    {
      for (octave_idx_type i = 0; i < a.length (); i++)
	{
	  OCTAVE_QUIT;
	  result(i) = std::pow (a(i), static_cast<int> (b));
	}
    }
  else
    {
      for (octave_idx_type i = 0; i < a.length (); i++)
	{
	  OCTAVE_QUIT;
	  result(i) = std::pow (a(i), b);
	}
    }

  return result;
}

// -*- 10 -*-
octave_value
elem_xpow (const ComplexNDArray& a, const NDArray& b)
{
  dim_vector a_dims = a.dims ();
  dim_vector b_dims = b.dims ();

  if (a_dims != b_dims)
    {
      gripe_nonconformant ("operator .^", a_dims, b_dims);
      return octave_value ();
    }

  ComplexNDArray result (a_dims);

  for (octave_idx_type i = 0; i < a.length (); i++)
    {
      OCTAVE_QUIT;
      double btmp = b(i);
      if (xisint (btmp))
	result(i) = std::pow (a(i), static_cast<int> (btmp));
      else
	result(i) = std::pow (a(i), btmp);
    }

  return result;
}

// -*- 11 -*-
octave_value
elem_xpow (const ComplexNDArray& a, const Complex& b)
{
  ComplexNDArray result (a.dims ());

  for (octave_idx_type i = 0; i < a.length (); i++)
    {
      OCTAVE_QUIT;
      result(i) = std::pow (a(i), b);
    }

  return result;
}

// -*- 12 -*-
octave_value
elem_xpow (const ComplexNDArray& a, const ComplexNDArray& b)
{
  dim_vector a_dims = a.dims ();
  dim_vector b_dims = b.dims ();

  if (a_dims != b_dims)
    {
      gripe_nonconformant ("operator .^", a_dims, b_dims);
      return octave_value ();
    }

  ComplexNDArray result (a_dims);

  for (octave_idx_type i = 0; i < a.length (); i++)
    {
      OCTAVE_QUIT;
      result(i) = std::pow (a(i), b(i));
    }

  return result;
}

/*
;;; Local Variables: ***
;;; mode: C++ ***
;;; End: ***
*/
