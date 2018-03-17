// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
//
// PadOS is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// PadOS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with PadOS. If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////

#include "SignalSystem.h"
#include "SignalTarget.h"

#ifndef NDEBUG
static int g_SignalCount = 0;
#endif  //!defined(NDEBUG)



/*! \page signals The Signal/Slot system.

\par General:
Signals are used for synchronous event passing. It can be used
by any system that need a flexible typesafe event system with
low overhead.
The core components in the system is "signals" and "slots".
A signal is a template based function object that is used to emit
an event. A slot is simply a normal/virtual/static class member
function or a global function that is called when the signal
are trigged.

\par
When declaring a signal, template-arguments are used to define
the prototype of the accepted slot functions. This makes it
possible for the compiler to do type-checking and emit errors at
compile-time if you try to connect an incompatible slot to a
signal. An unlimited number of slots can be connected to a signal
and a slot can be connected to an unlimited number of signals.
\par
If you want to connect a signal to a non-static class member (the
normal case) the class owning the slot function must inherit from
SignalTarget. The system will assure that you never get any kind
of "dangling pointers". If a signal is deleted all slots will
automatically be disconnected, and if an object that have connected
some of it's members to signals is deleted it will automatically
be disconnected from all subscribed signals.
\par
It is also safe to delete the signal or any objects connected to
the signal from a slot-function. If multiple slots are connected
to a signal and one of the slots delete the signal the iteration
will be aborted safely. It is also safe for a slot-function to
connect new slots to the signal or disconnect itself or other
slots from the signal. Great care has gone into making sure no
action will ever disrupt the iteration of slots connected to
a signal no matter what the slot functions do to the signals.
\par
If a slot-function connects another slot to the signal the
new slot will not be called until the next time the signal is
trigged. If a slot-function disconnect another slot the other
slot might or might not have been called. No assumptions should
be made about what order the different slots connected to a
signal is called in. If a slot-function deletes the signal
all slots will be disconnected and no more slots will be called.


\par Signals and VFConnectors:
There are two types of signals. Normal signals and vf-connectors.
Both types keeps a list of connected slots and overload the function
operator. The main difference is that while a Signal will iterate
through and call all it's slots when trigged, a VFConnector will only call
the last slot connected. Since a Signal might end up calling multiple
functions it can not forward a return value to the caller of the
signal. And it does not have any mechanism for collecting the return
values from all the slot functions. The VFConnector however only
call one of it's slots and will forward the return value from that
function.
\par
Both Signals and VFConnectors are declared the same way. They are template
classes that take 1 to N template arguments. The first template argument is
the return type and must always be specified (also for Signals even though
it is almost always \b void). Then you can specify null or more argument types.

To declare a signal that forward one argument you write it like this:

\code
Signal< void, const Point& > SignalMouseMoved;
\endcode

To connect to the signal you would write:

\code
SignalMouseMoved.Connect( this, &ClassName::SomeNormalOrVirtualMethod );
\endcode

or

\code
SignalCharacterMoved.Connect( &SomeGlobalOrStaticFunction );
\endcode


To send this signal you would write:

\code
SignalCharacterMoved( Point( x, y ) );
\endcode

*/


