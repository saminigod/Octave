## Copyright (C) 1996, 1997 Auburn University.  All Rights Reserved
##
## This file is part of Octave. 
##
## Octave is free software; you can redistribute it and/or modify it 
## under the terms of the GNU General Public License as published by the 
## Free Software Foundation; either version 2, or (at your option) any 
## later version. 
## 
## Octave is distributed in the hope that it will be useful, but WITHOUT 
## ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
## FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
## for more details.
## 
## You should have received a copy of the GNU General Public License 
## along with Octave; see the file COPYING.  If not, write to the Free 
## Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111 USA. 
 
## -*- texinfo -*-
## @deftypefn {Function File } {[@var{K}, @var{Q}, @var{P}, @var{Ee}, @var{Er}] =} lqg(@var{sys}, @var{Sigw}, @var{Sigv}, @var{Q}, @var{R}, @var{in_idx})
## Design a linear-quadratic-gaussian optimal controller for the system
## @example
## dx/dt = A x + B u + G w       [w]=N(0,[Sigw 0    ])
##     y = C x + v               [v]  (    0   Sigv ])
## @end example
## or
## @example 
## x(k+1) = A x(k) + B u(k) + G w(k)       [w]=N(0,[Sigw 0    ])
##   y(k) = C x(k) + v(k)                  [v]  (    0   Sigv ])
## @end example
## 
## @strong{Inputs}
## @table @var
## @item  sys
## system data structure
## @item  Sigw, Sigv
## intensities of independent Gaussian noise processes (as above)
## @item  Q, R
## state, control weighting respectively.  Control ARE is
## @item  in_idx
## indices of controlled inputs
## 
##      default: last dim(R) inputs are assumed to be controlled inputs, all
##               others are assumed to be noise inputs.
## @end table
## @strong{Outputs}
## @table @var
## @item    K
## system data structure format LQG optimal controller
## (Obtain A,B,C matrices with @code{sys2ss}, @code{sys2tf}, or @code{sys2zp} as appropriate)
## @item    P
## Solution of control (state feedback) algebraic Riccati equation
## @item    Q
## Solution of estimation algebraic Riccati equation
## @item    Ee
## estimator poles
## @item    Es
## controller poles
## @end table
## @end deftypefn

## See also:  h2syn, lqe, lqr

function [K,Q1,P1,Ee,Er] = lqg(sys,Sigw,Sigv,Q,R,input_list)

  ## Written by A. S. Hodel August 1995; revised for new system format
  ## August 1996

  if ( (nargin < 5) | (nargin > 6))
    usage("[K,Q1,P1,Ee,Er] = lqg(sys,Sigw, Sigv,Q,R{,input_list})");

  elseif(!is_struct(sys) )
    error("sys must be in system data structure");
  endif

  DIG = is_digital(sys);
  [A,B,C,D,tsam,n,nz,stname,inname,outname] = sys2ss(sys);
  [n,nz,nin,nout] = sysdimensions(sys);
  if(nargin == 5)
    ## construct default input_list
    input_list = (columns(Sigw)+1):nin;
  endif

  if( !(n+nz) )
      error(["lqg: 0 states in system"]);

  elseif(nin != columns(Sigw)+ columns(R))
    error(["lqg: sys has ",num2str(nin)," inputs, dim(Sigw)=", ...
	  num2str(columns(Sigw)),", dim(u)=",num2str(columns(R))])

  elseif(nout != columns(Sigv))
    error(["lqg: sys has ",num2str(nout)," outputs, dim(Sigv)=", ...
	  num2str(columns(Sigv)),")"])
  elseif(length(input_list) != columns(R))
    error(["lqg: length(input_list)=",num2str(length(input_list)), ...
	  ", columns(R)=", num2str(columns(R))]);
  endif

  varname = list("Sigw","Sigv","Q","R");
  for kk=1:length(varname);
    eval(sprintf("chk = is_square(%s);",nth(varname,kk)));
    if(! chk ) error("lqg: %s is not square",nth(varname,kk)); endif
  endfor

  ## permute (if need be)
  if(nargin == 6)
    all_inputs = sysreorder(nin,input_list);
    B = B(:,all_inputs);
    inname = inname(all_inputs);
  endif

  ## put parameters into correct variables
  m1 = columns(Sigw);
  m2 = m1+1;
  G = B(:,1:m1);
  B = B(:,m2:nin);

  ## now we can just do the design; call dlqr and dlqe, since all matrices
  ## are not given in Cholesky factor form (as in h2syn case)
  if(DIG)
    [Ks, P1, Er] = dlqr(A,B,Q,R);
    [Ke, Q1, jnk, Ee] = dlqe(A,G,C,Sigw,Sigv);
  else
    [Ks, P1, Er] = lqr(A,B,Q,R);
    [Ke, Q1, Ee] = lqe(A,G,C,Sigw,Sigv);
  endif
  Ac = A - Ke*C - B*Ks;
  Bc = Ke;
  Cc = -Ks;
  Dc = zeros(rows(Cc),columns(Bc));

  ## fix state names
  stname1 = strappend(stname,"_e");

  ## fix controller output names
  outname1 = strappend(inname(m2:nin),"_K");

  ## fix controller input names
  inname1 = strappend(outname,"_K");

  if(DIG)
    K = ss2sys(Ac,Bc,Cc,Dc,tsam,n,nz,stname1,inname1,outname1,1:rows(Cc));
  else
    K = ss2sys(Ac,Bc,Cc,Dc,tsam,n,nz,stname,inname1,outname1);
  endif

endfunction
