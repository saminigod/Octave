## Copyright (C) 1993, 1994, 1995 John W. Eaton
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
## @deftypefn {Function File } { @var{dsys} =} c2d (@var{sys}@{, @var{opt}, @var{T}@})
## @deftypefnx {Function File } { @var{dsys} =} c2d (@var{sys}@{, @var{T}@})
## 
## @strong{Inputs}
## @table @var
## @item sys
##  system data structure (may have both continuous time and discrete time subsystems)
## @item opt
## string argument; conversion option (optional argument; 
## may be omitted as shown above) 
## @table @code
## @item "ex" 
## use the matrix exponential (default)
## @item "bi" 
## use the bilinear transformation
## @end table
## @example
##     2(z-1)
## s = -----
##     T(z+1)
## @end example
## FIXME: This option exits with an error if @var{sys} is not purely 
## continuous. (The @code{ex} option can handle mixed systems.)
## @item @var{T}
## sampling time; required if sys is purely continuous.
## 
## @strong{Note} If the 2nd argument is not a string, @code{c2d} assumes that
## the 2nd argument is @var{T} and performs appropriate argument checks.
## @end table
## 
## @strong{Outputs}
## @var{dsys} discrete time equivalent via zero-order hold, 
## sample each @var{T} sec.
## 
## converts the system data structure describing
## @example
## .
## x = Ac x + Bc u
## @end example
## into a discrete time equivalent model
## @example
## x[n+1] = Ad x[n] + Bd u[n]
## @end example
## via the matrix exponential or bilinear transform
## 
## @strong{Note} This function adds the suffix  @code{_d}
## to the names of the new discrete states.   
## @end deftypefn

function dsys = c2d (sys, opt, T)

  ## Written by R.B. Tenison (btenison@eng.auburn.edu)
  ## October 1993
  ## Updated by John Ingram for system data structure August 1996

  ## parse input arguments
  if(nargin < 1 | nargin > 3)
    usage("dsys=c2d(sys[,T])");
  elseif (!is_struct(sys))
    error("sys must be a system data structure");
  elseif (nargin == 1)
    opt = "ex";
  elseif (nargin == 2 & !isstr(opt) )
    T = opt;
    opt = "ex";
  endif

  ## check if sampling period T was passed.
  Ts = sysgettsam(sys);
  if(!exist("T"))
    T = Ts;
    if(T == 0)
      error("sys is purely continuous; no sampling period T provided");
    endif
  elseif (T != Ts & Ts > 0)
    warning(["c2d: T=",num2str(T),", system tsam==",num2str(Ts), ...
      ": using T=", num2str(min(T,Ts))]);
    T = min(T,Ts);
  endif

  if (!is_sample(T))
    error("sampling period T must be a postive, real scalar");
  elseif( ! (strcmp(opt,"ex") | strcmp(opt,"bi") ) )
    error(["illegal option passed: ",opt])
  endif

  sys = sysupdate(sys,"ss");
  [n,nz,m,p] = sysdimensions(sys);

  if (n == 0)
    warning("c2d: sys has no continuous states; setting outputs to discrete");
    dsys = syssetsignals(sys,"yd",ones(1:p));
  elseif(strcmp(opt,"ex"))
    ## construct new state-space (a,b,c,d) for continuous subsystem
    [csys,Acd] = syscont(sys);   	# extract continuous subsystem
    [csys_a, csys_b, csys_c, csys_d] = sys2ss(csys);
    [ sys_a,  sys_b,  sys_c,  sys_d] = sys2ss( sys);
    if(isempty(Acd))                Bmat = sys_b;
    elseif(isempty(csys_b))         Bmat = Acd;
    else                            Bmat = [Acd, csys_b];     endif
    
    row_zer = columns(Bmat);
    csysn = sysdimensions(csys);
    col_zer = csysn + row_zer;

    [csysa,csysb,csysc,csysd] = sys2ss(csys);
    if(isempty(Bmat) )
      warning("c2d: no inputs to continuous subsystem.");
      mat = csysa;
    else
      mat = [csysa, Bmat ; zeros( row_zer,col_zer) ];
    endif

    matexp = expm(mat * T);
  
    Abar = matexp( 1:csysn , 1:(csysn + columns(Acd)) );  
    Bbar = matexp( 1:csysn , (columns(Abar) + 1):columns(matexp) );

    newnz = rows(Abar);
    outlist = ones(1,rows(csysc));
    [stnames,innames,outnames] = sysgetsignals(csys);
    dsys = ss2sys(Abar,Bbar,csysc,csysd,T,0,newnz,stnames,innames, ...
	outnames,outlist);
    ## rename states
    for ii=1:newnz
      strval = sprintf("%s_d",sysgetsignals(dsys,"st",ii,1));
      dsys = syssetsignals(dsys,"st",strval,ii);
    endfor

  elseif(strcmp(opt,"bi"))
    if(is_digital(sys))
      error("c2d: system is already digital")
    else
      ## convert with bilinear transform
      [a,b,c,d,tsam,n,nz,stname,inname,outname,yd] = sys2ss(sys);
      IT = (2/T)*eye(size(a));
      A = (IT+a)/(IT-a);
      iab = (IT-a)\b;
      tk=2/sqrt(T);
      B = tk*iab;
      C = tk*(c/(IT-a));
      D = d + (c*iab);
      stnamed = strappend(stname,"_d");
      dsys = ss2sys(A,B,C,D,T,0,rows(A),stnamed,inname,outname);
    endif
  else
    error(["Bad option=",opt])
  endif
  
endfunction
