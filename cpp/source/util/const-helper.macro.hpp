// no #pragma once intentionally

// Tired of writing this?
//
// > struct Foo
// > {
// >     auto& get() const
// >     {
// >         return value;
// >     }
// >
// >     auto& get()
// >     {
// >         return value;
// >     }
// >
// > private:
// >    int value;
// > };
//
// CONST_HELPER comes to the rescue! You can now write this:
//
// > struct Foo
// > {
// >     CONST_HELPER(
// >         auto& get() CONST
// >         {
// >             return value;
// >         }
// >     );
// >
// > private:
// >    int value;
// > };




// DON'T LOOK ANY FURTHUR.
// IT'S BLACK MAGIC.




// The MIT License
// Copyright (C) Giumo Clanjor (Weiju Lan) 2015-2018.
//
// Permission is hereby granted, free  of charge, to any person obtaining
// a  copy  of this  software  and  associated  documentation files  (the
// "Software"), to  deal in  the Software without  restriction, including
// without limitation  the rights to  use, copy, modify,  merge, publish,
// distribute,  sublicense, and/or sell  copies of  the Software,  and to
// permit persons to whom the Software  is furnished to do so, subject to
// the following conditions:
//
// The  above  copyright  notice  and  this permission  notice  shall  be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE  IS  PROVIDED  "AS  IS", WITHOUT  WARRANTY  OF ANY  KIND,
// EXPRESS OR  IMPLIED, INCLUDING  BUT NOT LIMITED  TO THE  WARRANTIES OF
// MERCHANTABILITY,    FITNESS    FOR    A   PARTICULAR    PURPOSE    AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE,  ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#define CONST_HELPER_IMPL_KP(...)       __VA_ARGS__     // KP = Kill Parenthesis
#define CONST_HELPER_IMPL(BEFORE, ...) \
    CONST_HELPER_IMPL_KP BEFORE         __VA_ARGS__ \
    CONST_HELPER_IMPL_KP BEFORE  const  __VA_ARGS__
#define CONST_HELPER(...)   CONST_HELPER_IMPL((__VA_ARGS__)
#define CONST   ),

