## Copyright (C) 1996,1998 Auburn University.  All Rights Reserved.
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
## @deftypefn {Function File } { @var{retsys} =} sysdup (@var{Asys}, @var{out_idx}, @var{in_idx})
##  Duplicate specified input/output connections of a system
## 
## @strong{Inputs}
## @table @var
## @item Asys
##  system data structure (@xref{ss2sys})
## @item out_idx,in_idx
##  list of connections indices; 
##        duplicates are made of @var{y(out_idx(ii))} and @var{u(in_idx(ii))}.
## @end table
## 
## @strong{Outputs}
## @var{retsys}: resulting closed loop system:
##     duplicated i/o names are appended with a @code{"+"} suffix.
## 
## 
## @strong{Method}
## @code{sysdup} creates copies of selected inputs and outputs as
##  shown below.  u1/y1 is the set of original inputs/outputs, and 
##  u2,y2 is the set of duplicated inputs/outputs in the order specified
##  in @var{in_idx}, @var{out_idx}, respectively
## @example
## @group
##           ____________________
## u1  ----->|                  |----> y1
##           |       Asys       |
## u2 ------>|                  |----->y2 
## (in_idx)  -------------------| (out_idx)
## @end group
## @end example
## 
## @end deftypefn

function retsys = sysdup(Asys,output_list,input_list)

  ## A. S. Hodel August 1995
  ## modified by John Ingram July 1996

  if( nargin != 3)
    usage("retsys = sysdup(Asys,output_list,input_list)");
  endif

  if( !is_struct(Asys))
    error("Asys must be a system data structure (see ss2sys, tf2sys, or zp2sys)")
  endif

  Asys = sysupdate(Asys,"ss");
  [nn,nz,mm,pp] = sysdimensions(Asys);
  [aa,bb,cc,dd] = sys2ss(Asys);

  ## first duplicate inputs
  if(is_vector(input_list))
    for ii=1:length(input_list);
      bb(:,mm+ii) = bb(:,input_list(ii));
      dd(:,mm+ii) = dd(:,input_list(ii));
    end
  elseif(!isempty(input_list))
    error("input_list must be a vector or empty");
  endif


  ## now duplicate outputs
  osize = min(size(output_list));
  if(osize == 1)
    for ii=1:length(output_list);
      cc(pp+ii,:) = cc(output_list(ii),:);
      dd(pp+ii,:) = dd(output_list(ii),:);
    end
  elseif(osize != 0)
    error("output_list must be a vector or empty");
  endif
  
  [stnam,innam,outnam,yd] = sysgetsignals(Asys);
  tsam = sysgettsam(Asys);

  ## pack system and then rename signals
  retsys = ss2sys(aa,bb,cc,dd,tsam,nn,nz);
  retsys = syssetsignals(retsys,"in",innam,1:mm);
  retsys = syssetsignals(retsys,"out",outnam,1:pp);
  retsys = syssetsignals(retsys,"yd",yd,1:pp);

  ## update added input names
  for ii=(mm+1):(mm+length(input_list))
    onum = input_list(ii-mm);
    strval = sprintf("%s(dup)",sysgetsignals(retsys,"in",onum,1) );
    retsys = syssetsignals(retsys,"in",strval,ii);
  endfor

  ## update added output names/discrete flags
  ## give default names to the added outputs
  for jj=(pp+1):(pp+length(output_list))
    onum = output_list(jj-pp);
    strval = sprintf("%s(dup)",sysgetsignals(retsys,"out",onum,1) );
    retsys = syssetsignals(retsys,"out",strval,jj);
    dflg = sysgetsignals(retsys,"yd",onum);
    retsys = syssetsignals(retsys,"yd",dflg,jj);
  endfor

endfunction
