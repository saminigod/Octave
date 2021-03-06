/*

Copyright (C) 1993-2016 John W. Eaton
Copyright (C) 2009-2010 VZLU Prague

This file is part of Octave.

Octave is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Octave is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Octave; see the file COPYING.  If not, see
<http://www.gnu.org/licenses/>.

*/

#if ! defined (octave_action_container_h)
#define octave_action_container_h 1

#include "octave-config.h"

// This class allows registering actions in a list for later
// execution, either explicitly or when the container goes out of
// scope.

// FIXME: is there a better name for this class?

class
action_container
{
public:

  // A generic unwind_protect element.  Knows how to run itself and
  // discard itself.  Also, contains a pointer to the next element.
  class elem
  {
  public:

    friend class action_container;

    elem (void) { }

    // No copying!

    elem (const elem&) = delete;

    elem& operator = (const elem&) = delete;

    virtual ~elem (void) = default;

    virtual void run (void) { }
  };

  // An element that merely runs a void (*)(void) function.

  class fcn_elem : public elem
  {
  public:

    fcn_elem (void (*fptr) (void))
      : e_fptr (fptr) { }

    void run (void) { e_fptr (); }

  private:

    void (*e_fptr) (void);
  };

  // An element that stores a variable of type T along with a void (*) (T)
  // function pointer, and calls the function with the parameter.

  template <typename T>
  class fcn_arg_elem : public elem
  {
  public:

    fcn_arg_elem (void (*fcn) (T), T arg)
      : e_fcn (fcn), e_arg (arg) { }

    // No copying!

    fcn_arg_elem (const fcn_arg_elem&) = delete;

    fcn_arg_elem& operator = (const fcn_arg_elem&) = delete;

    void run (void) { e_fcn (e_arg); }

  private:

    void (*e_fcn) (T);

    T e_arg;
  };

  // An element that stores a variable of type T along with a
  // void (*) (const T&) function pointer, and calls the function with
  // the parameter.

  template <typename T>
  class fcn_crefarg_elem : public elem
  {
  public:

    fcn_crefarg_elem (void (*fcn) (const T&), const T& arg)
      : e_fcn (fcn), e_arg (arg) { }

    void run (void) { e_fcn (e_arg); }

  private:

    void (*e_fcn) (const T&);

    T e_arg;
  };

  // An element for calling a member function.

  template <typename T>
  class method_elem : public elem
  {
  public:

    method_elem (T *obj, void (T::*method) (void))
      : e_obj (obj), e_method (method) { }

    // No copying!

    method_elem (const method_elem&) = delete;

    method_elem operator = (const method_elem&) = delete;

    void run (void) { (e_obj->*e_method) (); }

  private:

    T *e_obj;

    void (T::*e_method) (void);
  };

  // An element for calling a member function with a single argument

  template <typename T, typename A>
  class method_arg_elem : public elem
  {
  public:

    method_arg_elem (T *obj, void (T::*method) (A), A arg)
      : e_obj (obj), e_method (method), e_arg (arg) { }

    // No copying!

    method_arg_elem (const method_arg_elem&) = delete;

    method_arg_elem operator = (const method_arg_elem&) = delete;

    void run (void) { (e_obj->*e_method) (e_arg); }

  private:

    T *e_obj;

    void (T::*e_method) (A);

    A e_arg;
  };

  // An element for calling a member function with a single argument

  template <typename T, typename A>
  class method_crefarg_elem : public elem
  {
  public:

    method_crefarg_elem (T *obj, void (T::*method) (const A&), const A& arg)
      : e_obj (obj), e_method (method), e_arg (arg) { }

    // No copying!

    method_crefarg_elem (const method_crefarg_elem&) = delete;

    method_crefarg_elem operator = (const method_crefarg_elem&) = delete;

    void run (void) { (e_obj->*e_method) (e_arg); }

  private:

    T *e_obj;

    void (T::*e_method) (const A&);

    A e_arg;
  };

  // An element that stores arbitrary variable, and restores it.

  template <typename T>
  class restore_var_elem : public elem
  {
  public:

    restore_var_elem (T& ref, const T& val)
      : e_ptr (&ref), e_val (val) { }

    // No copying!

    restore_var_elem (const restore_var_elem&) = delete;

    restore_var_elem& operator = (const restore_var_elem&) = delete;

    void run (void) { *e_ptr = e_val; }

  private:

    T *e_ptr, e_val;
  };

  // Deletes a class allocated using new.

  template <typename T>
  class delete_ptr_elem : public elem
  {
  public:

    delete_ptr_elem (T *ptr)
      : e_ptr (ptr) { }

    // No copying!

    delete_ptr_elem (const delete_ptr_elem&) = delete;

    delete_ptr_elem operator = (const delete_ptr_elem&) = delete;

    void run (void) { delete e_ptr; }

  private:

    T *e_ptr;
  };

  action_container (void) { }

  // No copying!

  action_container (const action_container&) = delete;

  action_container& operator = (const action_container&) = delete;

  virtual ~action_container (void) = default;

  virtual void add (elem *new_elem) = 0;

  // Call to void func (void).
  void add_fcn (void (*fcn) (void))
  {
    add (new fcn_elem (fcn));
  }

  // Call to void func (T).
  template <typename T>
  void add_fcn (void (*action) (T), T val)
  {
    add (new fcn_arg_elem<T> (action, val));
  }

  // Call to void func (const T&).
  template <typename T>
  void add_fcn (void (*action) (const T&), const T& val)
  {
    add (new fcn_crefarg_elem<T> (action, val));
  }

  // Call to T::method (void).
  template <typename T>
  void add_method (T *obj, void (T::*method) (void))
  {
    add (new method_elem<T> (obj, method));
  }

  // Call to T::method (A).
  template <typename T, typename A>
  void add_method (T *obj, void (T::*method) (A), A arg)
  {
    add (new method_arg_elem<T, A> (obj, method, arg));
  }

  // Call to T::method (const A&).
  template <typename T, typename A>
  void add_method (T *obj, void (T::*method) (const A&), const A& arg)
  {
    add (new method_crefarg_elem<T, A> (obj, method, arg));
  }

  // Call to delete (T*).

  template <typename T>
  void add_delete (T *obj)
  {
    add (new delete_ptr_elem<T> (obj));
  }

  // Protect any variable.
  template <typename T>
  void protect_var (T& var)
  {
    add (new restore_var_elem<T> (var, var));
  }

  // Protect any variable, value given.
  template <typename T>
  void protect_var (T& var, const T& val)
  {
    add (new restore_var_elem<T> (var, val));
  }

  operator bool (void) const { return ! empty (); }

  virtual void run_first (void) = 0;

  void run (size_t num)
  {
    if (num > size ())
      num = size ();

    for (size_t i = 0; i < num; i++)
      run_first ();
  }

  void run (void) { run (size ()); }

  virtual void discard_first (void) = 0;

  void discard (size_t num)
  {
    if (num > size ())
      num = size ();

    for (size_t i = 0; i < num; i++)
      discard_first ();
  }

  void discard (void) { discard (size ()); }

  virtual size_t size (void) const = 0;

  bool empty (void) const { return size () == 0; }
};

#endif

