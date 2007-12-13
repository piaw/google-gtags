// Copyright 2007 Google Inc. All Rights Reserved.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// Author: nigdsouza@google.com (Nigel D'souza)
//
// A callback library that supports callbacks of upto 2 arguments.
// This library supports permanent and non-permanent callbacks.  Permanent
// callbacks can be run multiple times and require explicit deletion.
// Non-permanent callbacks can only be run once, after which they are
// immediately, automatically deleted.
// The library also supports callbacks with upto 3 prebound arguments.
//
// Examples:
//   string s = "Hello World!";
//   {
//     Callback0<int> *cb0 =
//         CallbackFactory::Create(&s, &string::length);
//     cb0->Run();         // returns 12
//   }
//   {
//     Callback1<char, int> *cb1 =
//         CallbackFactory::Create(&s, &string::reserve);
//     cb1->Run(15);       // reserves space in s for 15 characters
//   }
//   {
//     Callback2<int, char, int> *cb =
//         CallbackFactory::CreatePermanent(&s, &string::find);
//     cb2->Run('o', 5);   // returns 7
//     cb2->Run('l', 4);   // returns 9
//     cb2->Run('r', 1);   // returns 8
//     delete cb2;         // required for permanent Callbacks
//   }
//
// We define the various templated callback interfaces Callback*<>.
//
// We then define various implementation of the templated callback interfaces.
//   For example, Callback2<..> has a member-function implementation in
//   __MemberCallback_0_2<..>.
// The internal classes are suffixed with _p_a where p is the number of prebound
// arguments accepted and a is the number of run arguments required.
//
// These templates all take a 'permanent' parameter that indicates whether the
// Callback will be a permanent one.  The decision of whether or not the
// Callback will be permanent is known at the point of the call to 'new'.
// Since we are using templates, we can use that knowledge to dynamically
// declare the class.  Consequently, the compiler has more information that it
// can use to optimize out code.
//   For example, in the Run function, the compiler can remove the 'delete this'
//   statement from the class definition if the Callback is permanent.
//
// Callbacks that return void require special treatment.  We use partial
// template specialization to allow for special treatment while maintaining
// a similar interface.
//
// We define a CallbackFactory as another layer of indirection.  The
// CallbackFactory is the way to instantiate a Callback.  We take advantage of
// templating and function overloading to ensure that all Callbacks can be
// instantiated through CallbackFactory::Create(..), for non-permanent callbacks
// or CallbackFactory::CreatePermanent(..) for permanent callbacks.  The
// abstraction provided by CallbackFactory affords extensions to callbacks
// without altering the interface.
//
// Due to the repetitive nature of the class definitions, we use macros to
// define the Run function.  We can add/expand the macros as needed.
//
// Differences with internal callback library:
//   * No support for const-ness

#ifndef TOOLS_TAGS_CALLBACK_H__
#define TOOLS_TAGS_CALLBACK_H__

#include "tagsutil.h"

// Helpful macros for defining all the classes
#define __DEFINE_CALLBACK_CLASS(name, run_args)        \
  class name : public Callback {                       \
   public:                                             \
    virtual ~name() {}                                 \
    virtual R Run run_args = 0;                        \
   protected:                                          \
    name() {}                                          \
   private:                                            \
    DISALLOW_EVIL_CONSTRUCTORS(name);                  \
  };

#define __DEFINE_R_MEMBER_RUN_FUNCTION(run_args, fn_args)       \
  virtual R Run run_args {                                      \
    R result = (obj_->*fn_)fn_args;                             \
    if (!permanent)                                             \
      delete this;                                              \
    return result;                                              \
  }

#define __DEFINE_void_MEMBER_RUN_FUNCTION(run_args, fn_args)    \
  virtual void Run run_args {                                   \
    (obj_->*fn_)fn_args;                                        \
    if (!permanent)                                             \
      delete this;                                              \
  }

#define __DEFINE_R_STATIC_RUN_FUNCTION(run_args, fn_args)       \
  virtual R Run run_args {                                      \
    R result = (*fn_)fn_args;                                   \
    if (!permanent)                                             \
      delete this;                                              \
    return result;                                              \
  }

#define __DEFINE_void_STATIC_RUN_FUNCTION(run_args, fn_args)    \
  virtual void Run run_args {                                   \
    (*fn_)fn_args;                                              \
    if (!permanent)                                             \
      delete this;                                              \
  }

#define __DEFINE_ISREPEATABLE_FUNCTION()                        \
  virtual bool IsRepeatable() const { return permanent; }


#define __DEFINE_COMMON_MEMBER_CALLBACK_CLASS_BODY(             \
    name, voidness, fn_def_args, run_args, fn_args)             \
  friend class CallbackFactory;                                 \
 public:                                                        \
  __DEFINE_ISREPEATABLE_FUNCTION();                             \
 private:                                                       \
  DISALLOW_EVIL_CONSTRUCTORS(name);                             \
 protected:                                                     \
  typedef voidness (T::*FN) fn_def_args;                        \
  virtual ~name() {}                                            \
  __DEFINE_##voidness##_MEMBER_RUN_FUNCTION(run_args, fn_args); \
  T* obj_;                                                      \
  FN fn_

#define __DEFINE_COMMON_STATIC_CALLBACK_CLASS_BODY(             \
    name, voidness, fn_def_args, run_args, fn_args)             \
  friend class CallbackFactory;                                 \
 public:                                                        \
  __DEFINE_ISREPEATABLE_FUNCTION();                             \
 private:                                                       \
  DISALLOW_EVIL_CONSTRUCTORS(name);                             \
 protected:                                                     \
  typedef voidness (*FN) fn_def_args;                           \
  virtual ~name() {}                                            \
  __DEFINE_##voidness##_STATIC_RUN_FUNCTION(run_args, fn_args); \
  FN fn_


#define __DEFINE_MEMBER_CALLBACK_0_CLASS_BODY(          \
    name, voidness, fn_def_args, run_args, fn_args)     \
  __DEFINE_COMMON_MEMBER_CALLBACK_CLASS_BODY(           \
      name, voidness, fn_def_args, run_args, fn_args);  \
  name(T* obj, FN fn) : obj_(obj), fn_(fn) {}

#define __DEFINE_MEMBER_CALLBACK_1_CLASS_BODY(          \
    name, voidness, fn_def_args, run_args, fn_args)     \
  __DEFINE_COMMON_MEMBER_CALLBACK_CLASS_BODY(           \
      name, voidness, fn_def_args, run_args, fn_args);  \
  name(T* obj, FN fn, P1 p1) :                          \
      obj_(obj), fn_(fn), p1_(p1) {}                    \
  P1 p1_

#define __DEFINE_MEMBER_CALLBACK_2_CLASS_BODY(          \
    name, voidness, fn_def_args, run_args, fn_args)     \
  __DEFINE_COMMON_MEMBER_CALLBACK_CLASS_BODY(           \
      name, voidness, fn_def_args, run_args, fn_args);  \
  name(T* obj, FN fn, P1 p1, P2 p2) :                   \
      obj_(obj), fn_(fn), p1_(p1), p2_(p2) {}           \
  P1 p1_;                                               \
  P2 p2_

#define __DEFINE_MEMBER_CALLBACK_3_CLASS_BODY(          \
    name, voidness, fn_def_args, run_args, fn_args)     \
  __DEFINE_COMMON_MEMBER_CALLBACK_CLASS_BODY(           \
      name, voidness, fn_def_args, run_args, fn_args);  \
  name(T* obj, FN fn, P1 p1, P2 p2, P3 p3) :            \
      obj_(obj), fn_(fn), p1_(p1), p2_(p2), p3_(p3) {}  \
  P1 p1_;                                               \
  P2 p2_;                                               \
  P3 p3_


#define __DEFINE_STATIC_CALLBACK_0_CLASS_BODY(          \
    name, voidness, fn_def_args, run_args, fn_args)     \
  __DEFINE_COMMON_STATIC_CALLBACK_CLASS_BODY(           \
      name, voidness, fn_def_args, run_args, fn_args);  \
  name(FN fn) : fn_(fn) {}

#define __DEFINE_STATIC_CALLBACK_1_CLASS_BODY(          \
    name, voidness, fn_def_args, run_args, fn_args)     \
  __DEFINE_COMMON_STATIC_CALLBACK_CLASS_BODY(           \
      name, voidness, fn_def_args, run_args, fn_args);  \
  name(FN fn, P1 p1) :                                  \
      fn_(fn), p1_(p1) {}                               \
  P1 p1_

#define __DEFINE_STATIC_CALLBACK_2_CLASS_BODY(          \
    name, voidness, fn_def_args, run_args, fn_args)     \
  __DEFINE_COMMON_STATIC_CALLBACK_CLASS_BODY(           \
      name, voidness, fn_def_args, run_args, fn_args);  \
  name(FN fn, P1 p1, P2 p2) :                           \
      fn_(fn), p1_(p1), p2_(p2) {}                      \
  P1 p1_;                                               \
  P2 p2_

#define __DEFINE_STATIC_CALLBACK_3_CLASS_BODY(          \
    name, voidness, fn_def_args, run_args, fn_args)     \
  __DEFINE_COMMON_STATIC_CALLBACK_CLASS_BODY(           \
      name, voidness, fn_def_args, run_args, fn_args);  \
  name(FN fn, P1 p1, P2 p2, P3 p3) :                    \
      fn_(fn), p1_(p1), p2_(p2), p3_(p3) {}             \
  P1 p1_;                                               \
  P2 p2_;                                               \
  P3 p3_


namespace gtags {

// Base Callback interface
class Callback {
 public:
  virtual bool IsRepeatable() const = 0;
};

// *** 0-Argument Callbacks *** //

// Templated 0-argument callback interface
template <typename R>
__DEFINE_CALLBACK_CLASS(Callback0, ())

// We define a Closure for similarity with the internal callback library
typedef Callback0<void> Closure;

// Templated 0-prebound 0-argument member-callback implementation
template <bool permanent, typename R, class T>
class __MemberCallback_0_0
    : public Callback0<R> {
  __DEFINE_MEMBER_CALLBACK_0_CLASS_BODY(
      __MemberCallback_0_0, R, (), (), ());
};
template <bool permanent, class T>
class __MemberCallback_0_0<permanent, void, T>
    : public Callback0<void> {
  __DEFINE_MEMBER_CALLBACK_0_CLASS_BODY(
      __MemberCallback_0_0, void, (), (), ());
};

// Templated 0-prebound 0-argument static-callback implementation
template <bool permanent, typename R>
class __StaticCallback_0_0
    : public Callback0<R> {
  __DEFINE_STATIC_CALLBACK_0_CLASS_BODY(
      __StaticCallback_0_0, R, (), (), ());
};
template <bool permanent>
class __StaticCallback_0_0<permanent, void>
    : public Callback0<void> {
  __DEFINE_STATIC_CALLBACK_0_CLASS_BODY(
      __StaticCallback_0_0, void, (), (), ());
};

// Templated 1-prebound 0-argument member-callback implementation
template <bool permanent, typename R, class T,
          typename P1>
class __MemberCallback_1_0
    : public Callback0<R> {
  __DEFINE_MEMBER_CALLBACK_1_CLASS_BODY(
      __MemberCallback_1_0, R, (P1), (), (p1_));
};
template <bool permanent, class T,
          typename P1>
class __MemberCallback_1_0<permanent, void, T, P1>
    : public Callback0<void> {
  __DEFINE_MEMBER_CALLBACK_1_CLASS_BODY(
      __MemberCallback_1_0, void, (P1), (), (p1_));
};

// Templated 1-prebound 0-argument static-callback implementation
template <bool permanent, typename R,
          typename P1>
class __StaticCallback_1_0
    : public Callback0<R> {
  __DEFINE_STATIC_CALLBACK_1_CLASS_BODY(
      __StaticCallback_1_0, R, (P1), (), (p1_));
};
template <bool permanent,
          typename P1>
class __StaticCallback_1_0<permanent, void, P1>
    : public Callback0<void> {
  __DEFINE_STATIC_CALLBACK_1_CLASS_BODY(
      __StaticCallback_1_0, void, (P1), (), (p1_));
};

// Templated 2-prebound 0-argument member-callback implementation
template <bool permanent, typename R, class T,
          typename P1, typename P2>
class __MemberCallback_2_0
    : public Callback0<R> {
  __DEFINE_MEMBER_CALLBACK_2_CLASS_BODY(
      __MemberCallback_2_0, R, (P1, P2), (), (p1_, p2_));
};
template <bool permanent, class T,
          typename P1, typename P2>
class __MemberCallback_2_0<permanent, void, T, P1, P2>
    : public Callback0<void> {
  __DEFINE_MEMBER_CALLBACK_2_CLASS_BODY(
      __MemberCallback_2_0, void, (P1, P2), (), (p1_, p2_));
};

// Templated 2-prebound 0-argument static-callback implementation
template <bool permanent, typename R,
          typename P1, typename P2>
class __StaticCallback_2_0
    : public Callback0<R> {
  __DEFINE_STATIC_CALLBACK_2_CLASS_BODY(
      __StaticCallback_2_0, R, (P1, P2), (), (p1_, p2_));
};
template <bool permanent,
          typename P1, typename P2>
class __StaticCallback_2_0<permanent, void, P1, P2>
    : public Callback0<void> {
  __DEFINE_STATIC_CALLBACK_2_CLASS_BODY(
      __StaticCallback_2_0, void, (P1, P2), (), (p1_, p2_));
};

// Templated 3-prebound 0-argument member-callback implementation
template <bool permanent, typename R, class T,
          typename P1, typename P2, typename P3>
class __MemberCallback_3_0
    : public Callback0<R> {
  __DEFINE_MEMBER_CALLBACK_3_CLASS_BODY(
      __MemberCallback_3_0, R, (P1, P2, P3), (), (p1_, p2_, p3_));
};
template <bool permanent, class T,
          typename P1, typename P2, typename P3>
class __MemberCallback_3_0<permanent, void, T, P1, P2, P3>
    : public Callback0<void> {
  __DEFINE_MEMBER_CALLBACK_3_CLASS_BODY(
      __MemberCallback_3_0, void, (P1, P2, P3), (), (p1_, p2_, p3_));
};

// Templated 3-prebound 0-argument static-callback implementation
template <bool permanent, typename R,
          typename P1, typename P2, typename P3>
class __StaticCallback_3_0
    : public Callback0<R> {
  __DEFINE_STATIC_CALLBACK_3_CLASS_BODY(
      __StaticCallback_3_0, R, (P1, P2, P3), (), (p1_, p2_, p3_));
};
template <bool permanent,
          typename P1, typename P2, typename P3>
class __StaticCallback_3_0<permanent, void, P1, P2, P3>
    : public Callback0<void> {
  __DEFINE_STATIC_CALLBACK_3_CLASS_BODY(
      __StaticCallback_3_0, void, (P1, P2, P3), (), (p1_, p2_, p3_));
};


// *** 1-Argument Callbacks *** //

// Templated 1-argument callback interface
template <typename R, typename A1>
__DEFINE_CALLBACK_CLASS(Callback1, (A1 a1))

// Templated 0-prebound 1-argument member-callback implementation
template <bool permanent, typename R, class T, typename A1>
class __MemberCallback_0_1
    : public Callback1<R, A1> {
  __DEFINE_MEMBER_CALLBACK_0_CLASS_BODY(
      __MemberCallback_0_1, R, (A1), (A1 a1), (a1));
};
template <bool permanent, class T, typename A1>
class __MemberCallback_0_1<permanent, void, T, A1>
    : public Callback1<void, A1> {
  __DEFINE_MEMBER_CALLBACK_0_CLASS_BODY(
      __MemberCallback_0_1, void, (A1), (A1 a1), (a1));
};

// Templated 0-prebound 1-argument static-callback implementation
template <bool permanent, typename R, typename A1>
class __StaticCallback_0_1
    : public Callback1<R, A1> {
  __DEFINE_STATIC_CALLBACK_0_CLASS_BODY(
      __StaticCallback_0_1, R, (A1), (A1 a1), (a1));
};
template <bool permanent, typename A1>
class __StaticCallback_0_1<permanent, void, A1>
    : public Callback1<void, A1> {
  __DEFINE_STATIC_CALLBACK_0_CLASS_BODY(
      __StaticCallback_0_1, void, (A1), (A1 a1), (a1));
};

// Templated 1-prebound 1-argument member-callback implementation
template <bool permanent, typename R, class T,
          typename P1, typename A1>
class __MemberCallback_1_1
    : public Callback1<R, A1> {
  __DEFINE_MEMBER_CALLBACK_1_CLASS_BODY(
      __MemberCallback_1_1, R, (P1, A1), (A1 a1), (p1_, a1));
};
template <bool permanent, class T,
          typename P1, typename A1>
class __MemberCallback_1_1<permanent, void, T, P1, A1>
    : public Callback1<void, A1> {
  __DEFINE_MEMBER_CALLBACK_1_CLASS_BODY(
      __MemberCallback_1_1, void, (P1, A1), (A1 a1), (p1_, a1));
};

// Templated 1-prebound 1-argument static-callback implementation
template <bool permanent, typename R,
          typename P1, typename A1>
class __StaticCallback_1_1
    : public Callback1<R, A1> {
  __DEFINE_STATIC_CALLBACK_1_CLASS_BODY(
      __StaticCallback_1_1, R, (P1, A1), (A1 a1), (p1_, a1));
};
template <bool permanent,
          typename P1, typename A1>
class __StaticCallback_1_1<permanent, void, P1, A1>
    : public Callback1<void, A1> {
  __DEFINE_STATIC_CALLBACK_1_CLASS_BODY(
      __StaticCallback_1_1, void, (P1, A1), (A1 a1), (p1_, a1));
};

// Templated 2-prebound 1-argument member-callback implementation
template <bool permanent, typename R, class T,
          typename P1, typename P2, typename A1>
class __MemberCallback_2_1
    : public Callback1<R, A1> {
  __DEFINE_MEMBER_CALLBACK_2_CLASS_BODY(
      __MemberCallback_2_1, R, (P1, P2, A1), (A1 a1), (p1_, p2_, a1));
};
template <bool permanent, class T,
          typename P1, typename P2, typename A1>
class __MemberCallback_2_1<permanent, void, T, P1, P2, A1>
    : public Callback1<void, A1> {
  __DEFINE_MEMBER_CALLBACK_2_CLASS_BODY(
      __MemberCallback_2_1, void, (P1, P2, A1), (A1 a1), (p1_, p2_, a1));
};

// Templated 2-prebound 1-argument static-callback implementation
template <bool permanent, typename R,
          typename P1, typename P2, typename A1>
class __StaticCallback_2_1
    : public Callback1<R, A1> {
  __DEFINE_STATIC_CALLBACK_2_CLASS_BODY(
      __StaticCallback_2_1, R, (P1, P2, A1), (A1 a1), (p1_, p2_, a1));
};
template <bool permanent,
          typename P1, typename P2, typename A1>
class __StaticCallback_2_1<permanent, void, P1, P2, A1>
    : public Callback1<void, A1> {
  __DEFINE_STATIC_CALLBACK_2_CLASS_BODY(
      __StaticCallback_2_1, void, (P1, P2, A1), (A1 a1), (p1_, p2_, a1));
};

// Templated 3-prebound 1-argument member-callback implementation
template <bool permanent, typename R, class T,
          typename P1, typename P2, typename P3, typename A1>
class __MemberCallback_3_1
    : public Callback1<R, A1> {
  __DEFINE_MEMBER_CALLBACK_3_CLASS_BODY(
      __MemberCallback_3_1, R, (P1, P2, P3, A1),
      (A1 a1), (p1_, p2_, p3_, a1));
};
template <bool permanent, class T,
          typename P1, typename P2, typename P3, typename A1>
class __MemberCallback_3_1<permanent, void, T, P1, P2, P3, A1>
    : public Callback1<void, A1> {
  __DEFINE_MEMBER_CALLBACK_3_CLASS_BODY(
      __MemberCallback_3_1, void, (P1, P2, P3, A1),
      (A1 a1), (p1_, p2_, p3_, a1));
};

// Templated 3-prebound 1-argument static-callback implementation
template <bool permanent, typename R,
          typename P1, typename P2, typename P3, typename A1>
class __StaticCallback_3_1
    : public Callback1<R, A1> {
  __DEFINE_STATIC_CALLBACK_3_CLASS_BODY(
      __StaticCallback_3_1, R, (P1, P2, P3, A1),
      (A1 a1), (p1_, p2_, p3_, a1));
};
template <bool permanent,
          typename P1, typename P2, typename P3, typename A1>
class __StaticCallback_3_1<permanent, void, P1, P2, P3, A1>
    : public Callback1<void, A1> {
  __DEFINE_STATIC_CALLBACK_3_CLASS_BODY(
      __StaticCallback_3_1, void, (P1, P2, P3, A1),
      (A1 a1), (p1_, p2_, p3_, a1));
};


// *** 2-Argument Callbacks *** //

// Templated 2-argument callback interface
template <typename R, typename A1, typename A2>
__DEFINE_CALLBACK_CLASS(Callback2, (A1 a1, A2 a2))

// Templated 0-prebound 2-argument member-callback implementation
template <bool permanent, typename R, class T, typename A1, typename A2>
class __MemberCallback_0_2
    : public Callback2<R, A1, A2> {
  __DEFINE_MEMBER_CALLBACK_0_CLASS_BODY(
      __MemberCallback_0_2, R, (A1, A2), (A1 a1, A2 a2), (a1, a2));
};
template <bool permanent, class T, typename A1, typename A2>
class __MemberCallback_0_2<permanent, void, T, A1, A2>
    : public Callback2<void, A1, A2> {
  __DEFINE_MEMBER_CALLBACK_0_CLASS_BODY(
      __MemberCallback_0_2, void, (A1, A2), (A1 a1, A2 a2), (a1, a2));
};

// Templated 0-prebound 2-argument static-callback implementation
template <bool permanent, typename R, typename A1, typename A2>
class __StaticCallback_0_2
    : public Callback2<R, A1, A2> {
  __DEFINE_STATIC_CALLBACK_0_CLASS_BODY(
      __StaticCallback_0_2, R, (A1, A2), (A1 a1, A2 a2), (a1, a2));
};
template <bool permanent, typename A1, typename A2>
class __StaticCallback_0_2<permanent, void, A1, A2>
    : public Callback2<void, A1, A2> {
  __DEFINE_STATIC_CALLBACK_0_CLASS_BODY(
      __StaticCallback_0_2, void, (A1, A2), (A1 a1, A2 a2), (a1, a2));
};

// Templated 1-prebound 2-argument member-callback implementation
template <bool permanent, typename R, class T,
          typename P1, typename A1, typename A2>
class __MemberCallback_1_2
    : public Callback2<R, A1, A2> {
  __DEFINE_MEMBER_CALLBACK_1_CLASS_BODY(
      __MemberCallback_1_2, R, (P1, A1, A2),
      (A1 a1, A2 a2), (p1_, a1, a2));
};
template <bool permanent, class T,
          typename P1, typename A1, typename A2>
class __MemberCallback_1_2<permanent, void, T, P1, A1, A2>
    : public Callback2<void, A1, A2> {
  __DEFINE_MEMBER_CALLBACK_1_CLASS_BODY(
      __MemberCallback_1_2, void, (P1, A1, A2),
      (A1 a1, A2 a2), (p1_, a1, a2));
};

// Templated 1-prebound 2-argument static-callback implementation
template <bool permanent, typename R,
          typename P1, typename A1, typename A2>
class __StaticCallback_1_2
    : public Callback2<R, A1, A2> {
  __DEFINE_STATIC_CALLBACK_1_CLASS_BODY(
      __StaticCallback_1_2, R, (P1, A1, A2),
      (A1 a1, A2 a2), (p1_, a1, a2));
};
template <bool permanent,
          typename P1, typename A1, typename A2>
class __StaticCallback_1_2<permanent, void, P1, A1, A2>
    : public Callback2<void, A1, A2> {
  __DEFINE_STATIC_CALLBACK_1_CLASS_BODY(
      __StaticCallback_1_2, void, (P1, A1, A2),
      (A1 a1, A2 a2), (p1_, a1, a2));
};

// Templated 2-prebound 2-argument member-callback implementation
template <bool permanent, typename R, class T,
          typename P1, typename P2, typename A1, typename A2>
class __MemberCallback_2_2
    : public Callback2<R, A1, A2> {
  __DEFINE_MEMBER_CALLBACK_2_CLASS_BODY(
      __MemberCallback_2_2, R, (P1, P2, A1, A2),
      (A1 a1, A2 a2), (p1_, p2_, a1, a2));
};
template <bool permanent, class T,
          typename P1, typename P2, typename A1, typename A2>
class __MemberCallback_2_2<permanent, void, T, P1, P2, A1, A2>
    : public Callback2<void, A1, A2> {
  __DEFINE_MEMBER_CALLBACK_2_CLASS_BODY(
      __MemberCallback_2_2, void, (P1, P2, A1, A2),
      (A1 a1, A2 a2), (p1_, p2_, a1, a2));
};

// Templated 2-prebound 2-argument static-callback implementation
template <bool permanent, typename R,
          typename P1, typename P2, typename A1, typename A2>
class __StaticCallback_2_2
    : public Callback2<R, A1, A2> {
  __DEFINE_STATIC_CALLBACK_2_CLASS_BODY(
      __StaticCallback_2_2, R, (P1, P2, A1, A2),
      (A1 a1, A2 a2), (p1_, p2_, a1, a2));
};
template <bool permanent,
          typename P1, typename P2, typename A1, typename A2>
class __StaticCallback_2_2<permanent, void, P1, P2, A1, A2>
    : public Callback2<void, A1, A2> {
  __DEFINE_STATIC_CALLBACK_2_CLASS_BODY(
      __StaticCallback_2_2, void, (P1, P2, A1, A2),
      (A1 a1, A2 a2), (p1_, p2_, a1, a2));
};

// Templated 3-prebound 2-argument member-callback implementation
template <bool permanent, typename R, class T,
          typename P1, typename P2, typename P3, typename A1, typename A2>
class __MemberCallback_3_2
    : public Callback2<R, A1, A2> {
  __DEFINE_MEMBER_CALLBACK_3_CLASS_BODY(
      __MemberCallback_3_2, R, (P1, P2, P3, A1, A2),
      (A1 a1, A2 a2), (p1_, p2_, p3_, a1, a2));
};
template <bool permanent, class T,
          typename P1, typename P2, typename P3, typename A1, typename A2>
class __MemberCallback_3_2<permanent, void, T, P1, P2, P3, A1, A2>
    : public Callback2<void, A1, A2> {
  __DEFINE_MEMBER_CALLBACK_3_CLASS_BODY(
      __MemberCallback_3_2, void, (P1, P2, P3, A1, A2),
      (A1 a1, A2 a2), (p1_, p2_, p3_, a1, a2));
};

// Templated 3-prebound 2-argument static-callback implementation
template <bool permanent, typename R,
          typename P1, typename P2, typename P3, typename A1, typename A2>
class __StaticCallback_3_2
    : public Callback2<R, A1, A2> {
  __DEFINE_STATIC_CALLBACK_3_CLASS_BODY(
      __StaticCallback_3_2, R, (P1, P2, P3, A1, A2),
      (A1 a1, A2 a2), (p1_, p2_, p3_, a1, a2));
};
template <bool permanent,
          typename P1, typename P2, typename P3, typename A1, typename A2>
class __StaticCallback_3_2<permanent, void, P1, P2, P3, A1, A2>
    : public Callback2<void, A1, A2> {
  __DEFINE_STATIC_CALLBACK_3_CLASS_BODY(
      __StaticCallback_3_2, void, (P1, P2, P3, A1, A2),
      (A1 a1, A2 a2), (p1_, p2_, p3_, a1, a2));
};


// CallbackFactory is used to instantiate various types of Callbacks.
// Templating and function overloading allow all Callbacks to be instantiated
// through the Create(..) methods and all permanent Callbacks to be instantiated
// through the CreatePermanent(..) methods.
class CallbackFactory {
 public:
  // Creators for 0-prebound 0-argument Callbacks
  template <typename R, class T>
  static Callback0<R>* Create(
      T* obj, R (T::*fn)()) {
    return new __MemberCallback_0_0<false, R, T>(obj, fn);
  }
  template <typename R, class T>
  static Callback0<R>* CreatePermanent(
      T* obj, R (T::*fn)()) {
    return new __MemberCallback_0_0<true, R, T>(obj, fn);
  }

  template <typename R>
  static Callback0<R>* Create(
      R (*fn)()) {
    return new __StaticCallback_0_0<false, R>(fn);
  }
  template <typename R>
  static Callback0<R>* CreatePermanent(
      R (*fn)()) {
    return new __StaticCallback_0_0<true, R>(fn);
  }

  // Creators for 1-prebound 0-argument Callbacks
  template <typename R, class T,
            typename P1>
  static Callback0<R>* Create(
      T* obj, R (T::*fn)(P1), P1 p1) {
    return new __MemberCallback_1_0<false, R, T, P1>(obj, fn, p1);
  }
  template <typename R, class T,
            typename P1>
  static Callback0<R>* CreatePermanent(
      T* obj, R (T::*fn)(P1), P1 p1) {
    return new __MemberCallback_1_0<true, R, T, P1>(obj, fn, p1);
  }

  template <typename R,
            typename P1>
  static Callback0<R>* Create(
      R (*fn)(P1), P1 p1) {
    return new __StaticCallback_1_0<false, R, P1>(fn, p1);
  }
  template <typename R,
            typename P1>
  static Callback0<R>* CreatePermanent(
      R (*fn)(P1), P1 p1) {
    return new __StaticCallback_1_0<true, R, P1>(fn, p1);
  }

  // Creators for 2-prebound 0-argument Callbacks
  template <typename R, class T,
            typename P1, typename P2>
  static Callback0<R>* Create(
      T* obj, R (T::*fn)(P1, P2), P1 p1, P2 p2) {
    return new __MemberCallback_2_0<false, R, T, P1, P2>(obj, fn, p1, p2);
  }
  template <typename R, class T,
            typename P1, typename P2>
  static Callback0<R>* CreatePermanent(
      T* obj, R (T::*fn)(P1, P2), P1 p1, P2 p2) {
    return new __MemberCallback_2_0<true, R, T, P1, P2>(obj, fn, p1, p2);
  }

  template <typename R,
            typename P1, typename P2>
  static Callback0<R>* Create(
      R (*fn)(P1, P2), P1 p1, P2 p2) {
    return new __StaticCallback_2_0<false, R, P1, P2>(fn, p1, p2);
  }
  template <typename R,
            typename P1, typename P2>
  static Callback0<R>* CreatePermanent(
      R (*fn)(P1, P2), P1 p1, P2 p2) {
    return new __StaticCallback_2_0<true, R, P1, P2>(fn, p1, p2);
  }

  // Creators for 3-prebound 0-argument Callbacks
  template <typename R, class T,
            typename P1, typename P2, typename P3>
  static Callback0<R>* Create(
      T* obj, R (T::*fn)(P1, P2, P3), P1 p1, P2 p2, P3 p3) {
    return new __MemberCallback_3_0<false, R, T, P1, P2, P3>(obj, fn, p1, p2, p3);
  }
  template <typename R, class T,
            typename P1, typename P2, typename P3>
  static Callback0<R>* CreatePermanent(
      T* obj, R (T::*fn)(P1, P2, P3), P1 p1, P2 p2, P3 p3) {
    return new __MemberCallback_3_0<true, R, T, P1, P2, P3>(obj, fn, p1, p2, p3);
  }

  template <typename R,
            typename P1, typename P2, typename P3>
  static Callback0<R>* Create(
      R (*fn)(P1, P2, P3), P1 p1, P2 p2, P3 p3) {
    return new __StaticCallback_3_0<false, R, P1, P2, P3>(fn, p1, p2, p3);
  }
  template <typename R,
            typename P1, typename P2, typename P3>
  static Callback0<R>* CreatePermanent(
      R (*fn)(P1, P2, P3), P1 p1, P2 p2, P3 p3) {
    return new __StaticCallback_3_0<true, R, P1, P2, P3>(fn, p1, p2, p3);
  }

  // Creators for 0-prebound 1-argument Callbacks
  template <typename R, class T, typename A1>
  static Callback1<R, A1>* Create(
      T* obj, R (T::*fn)(A1)) {
    return new __MemberCallback_0_1<false, R, T, A1>(obj, fn);
  }
  template <typename R, class T, typename A1>
  static Callback1<R, A1>* CreatePermanent(
      T* obj, R (T::*fn)(A1)) {
    return new __MemberCallback_0_1<true, R, T, A1>(obj, fn);
  }

  template <typename R, typename A1>
  static Callback1<R, A1>* Create(
      R (*fn)(A1)) {
    return new __StaticCallback_0_1<false, R, A1>(fn);
  }
  template <typename R, typename A1>
  static Callback1<R, A1>* CreatePermanent(
      R (*fn)(A1)) {
    return new __StaticCallback_0_1<true, R, A1>(fn);
  }

  // Creators for 1-prebound 1-argument Callbacks
  template <typename R, class T,
            typename P1, typename A1>
  static Callback1<R, A1>* Create(
      T* obj, R (T::*fn)(P1, A1), P1 p1) {
    return new __MemberCallback_1_1<false, R, T, P1, A1>(obj, fn, p1);
  }
  template <typename R, class T,
            typename P1, typename A1>
  static Callback1<R, A1>* CreatePermanent(
      T* obj, R (T::*fn)(P1, A1), P1 p1) {
    return new __MemberCallback_1_1<true, R, T, P1, A1>(obj, fn, p1);
  }

  template <typename R,
            typename P1, typename A1>
  static Callback1<R, A1>* Create(
      R (*fn)(P1, A1), P1 p1) {
    return new __StaticCallback_1_1<false, R, P1, A1>(fn, p1);
  }
  template <typename R,
            typename P1, typename A1>
  static Callback1<R, A1>* CreatePermanent(
      R (*fn)(P1, A1), P1 p1) {
    return new __StaticCallback_1_1<true, R, P1, A1>(fn, p1);
  }

  // Creators for 2-prebound 1-argument Callbacks
  template <typename R, class T,
            typename P1, typename P2, typename A1>
  static Callback1<R, A1>* Create(
      T* obj, R (T::*fn)(P1, P2, A1), P1 p1, P2 p2) {
    return new __MemberCallback_2_1<false, R, T, P1, P2, A1>(obj, fn, p1, p2);
  }
  template <typename R, class T,
            typename P1, typename P2, typename A1>
  static Callback1<R, A1>* CreatePermanent(
      T* obj, R (T::*fn)(P1, P2, A1), P1 p1, P2 p2) {
    return new __MemberCallback_2_1<true, R, T, P1, P2, A1>(obj, fn, p1, p2);
  }

  template <typename R,
            typename P1, typename P2, typename A1>
  static Callback1<R, A1>* Create(
      R (*fn)(P1, P2, A1), P1 p1, P2 p2) {
    return new __StaticCallback_2_1<false, R, P1, P2, A1>(fn, p1, p2);
  }
  template <typename R,
            typename P1, typename P2, typename A1>
  static Callback1<R, A1>* CreatePermanent(
      R (*fn)(P1, P2, A1), P1 p1, P2 p2) {
    return new __StaticCallback_2_1<true, R, P1, P2, A1>(fn, p1, p2);
  }

  // Creators for 3-prebound 1-argument Callbacks
  template <typename R, class T,
            typename P1, typename P2, typename P3, typename A1>
  static Callback1<R, A1>* Create(
      T* obj, R (T::*fn)(P1, P2, P3, A1), P1 p1, P2 p2, P3 p3) {
    return new __MemberCallback_3_1<false, R, T, P1, P2, P3, A1>(obj, fn, p1, p2, p3);
  }
  template <typename R, class T,
            typename P1, typename P2, typename P3, typename A1>
  static Callback1<R, A1>* CreatePermanent(
      T* obj, R (T::*fn)(P1, P2, P3, A1), P1 p1, P2 p2, P3 p3) {
    return new __MemberCallback_3_1<true, R, T, P1, P2, P3, A1>(obj, fn, p1, p2, p3);
  }

  template <typename R,
            typename P1, typename P2, typename P3, typename A1>
  static Callback1<R, A1>* Create(
      R (*fn)(P1, P2, P3, A1), P1 p1, P2 p2, P3 p3) {
    return new __StaticCallback_3_1<false, R, P1, P2, P3, A1>(fn, p1, p2, p3);
  }
  template <typename R,
            typename P1, typename P2, typename P3, typename A1>
  static Callback1<R, A1>* CreatePermanent(
      R (*fn)(P1, P2, P3, A1), P1 p1, P2 p2, P3 p3) {
    return new __StaticCallback_3_1<true, R, P1, P2, P3, A1>(fn, p1, p2, p3);
  }

  // Creators for 0-prebound 2-argument Callbacks
  template <typename R, class T, typename A1, typename A2>
  static Callback2<R, A1, A2>* Create(
      T* obj, R (T::*fn)(A1, A2)) {
    return new __MemberCallback_0_2<false, R, T, A1, A2>(obj, fn);
  }
  template <typename R, class T, typename A1, typename A2>
  static Callback2<R, A1, A2>* CreatePermanent(
      T* obj, R (T::*fn)(A1, A2)) {
    return new __MemberCallback_0_2<true, R, T, A1, A2>(obj, fn);
  }

  template <typename R, typename A1, typename A2>
  static Callback2<R, A1, A2>* Create(
      R (*fn)(A1, A2)) {
    return new __StaticCallback_0_2<false, R, A1, A2>(fn);
  }
  template <typename R, typename A1, typename A2>
  static Callback2<R, A1, A2>* CreatePermanent(
      R (*fn)(A1, A2)) {
    return new __StaticCallback_0_2<true, R, A1, A2>(fn);
  }

  // Creators for 1-prebound 2-argument Callbacks
  template <typename R, class T,
            typename P1, typename A1, typename A2>
  static Callback2<R, A1, A2>* Create(
      T* obj, R (T::*fn)(P1, A1, A2), P1 p1) {
    return new __MemberCallback_1_2<false, R, T, P1, A1, A2>(obj, fn, p1);
  }
  template <typename R, class T,
            typename P1, typename A1, typename A2>
  static Callback2<R, A1, A2>* CreatePermanent(
      T* obj, R (T::*fn)(P1, A1, A2), P1 p1) {
    return new __MemberCallback_1_2<true, R, T, P1, A1, A2>(obj, fn, p1);
  }

  template <typename R,
            typename P1, typename A1, typename A2>
  static Callback2<R, A1, A2>* Create(
      R (*fn)(P1, A1, A2), P1 p1) {
    return new __StaticCallback_1_2<false, R, P1, A1, A2>(fn, p1);
  }
  template <typename R,
            typename P1, typename A1, typename A2>
  static Callback2<R, A1, A2>* CreatePermanent(
      R (*fn)(P1, A1, A2), P1 p1) {
    return new __StaticCallback_1_2<true, R, P1, A1, A2>(fn, p1);
  }

  // Creators for 2-prebound 2-argument Callbacks
  template <typename R, class T,
            typename P1, typename P2, typename A1, typename A2>
  static Callback2<R, A1, A2>* Create(
      T* obj, R (T::*fn)(P1, P2, A1, A2), P1 p1, P2 p2) {
    return new __MemberCallback_2_2<false, R, T, P1, P2, A1, A2>(obj, fn, p1, p2);
  }
  template <typename R, class T,
            typename P1, typename P2, typename A1, typename A2>
  static Callback2<R, A1, A2>* CreatePermanent(
      T* obj, R (T::*fn)(P1, P2, A1, A2), P1 p1, P2 p2) {
    return new __MemberCallback_2_2<true, R, T, P1, P2, A1, A2>(obj, fn, p1, p2);
  }

  template <typename R,
            typename P1, typename P2, typename A1, typename A2>
  static Callback2<R, A1, A2>* Create(
      R (*fn)(P1, P2, A1, A2), P1 p1, P2 p2) {
    return new __StaticCallback_2_2<false, R, P1, P2, A1, A2>(fn, p1, p2);
  }
  template <typename R,
            typename P1, typename P2, typename A1, typename A2>
  static Callback2<R, A1, A2>* CreatePermanent(
      R (*fn)(P1, P2, A1, A2), P1 p1, P2 p2) {
    return new __StaticCallback_2_2<true, R, P1, P2, A1, A2>(fn, p1, p2);
  }

  // Creators for 3-prebound 2-argument Callbacks
  template <typename R, class T,
            typename P1, typename P2, typename P3, typename A1, typename A2>
  static Callback2<R, A1, A2>* Create(
      T* obj, R (T::*fn)(P1, P2, P3, A1, A2), P1 p1, P2 p2, P3 p3) {
    return new __MemberCallback_3_2<false, R, T, P1, P2, P3, A1, A2>(obj, fn, p1, p2, p3);
  }
  template <typename R, class T,
            typename P1, typename P2, typename P3, typename A1, typename A2>
  static Callback2<R, A1, A2>* CreatePermanent(
      T* obj, R (T::*fn)(P1, P2, P3, A1, A2), P1 p1, P2 p2, P3 p3) {
    return new __MemberCallback_3_2<true, R, T, P1, P2, P3, A1, A2>(obj, fn, p1, p2, p3);
  }

  template <typename R,
            typename P1, typename P2, typename P3, typename A1, typename A2>
  static Callback2<R, A1, A2>* Create(
      R (*fn)(P1, P2, P3, A1, A2), P1 p1, P2 p2, P3 p3) {
    return new __StaticCallback_3_2<false, R, P1, P2, P3, A1, A2>(fn, p1, p2, p3);
  }
  template <typename R,
            typename P1, typename P2, typename P3, typename A1, typename A2>
  static Callback2<R, A1, A2>* CreatePermanent(
      R (*fn)(P1, P2, P3, A1, A2), P1 p1, P2 p2, P3 p3) {
    return new __StaticCallback_3_2<true, R, P1, P2, P3, A1, A2>(fn, p1, p2, p3);
  }

 private:
  CallbackFactory();
  DISALLOW_EVIL_CONSTRUCTORS(CallbackFactory);
};

}

#endif  // TOOLS_TAGS_CALLBACK_H__
