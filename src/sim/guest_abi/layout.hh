/*
 * Copyright 2019 Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __SIM_GUEST_ABI_LAYOUT_HH__
#define __SIM_GUEST_ABI_LAYOUT_HH__

#include <type_traits>

#include "sim/guest_abi/definition.hh"

class ThreadContext;

namespace GuestABI
{

/*
 * Position may need to be initialized based on the ThreadContext, for instance
 * to find out where the stack pointer is initially.
 */
template <typename ABI, typename Enabled=void>
struct PositionInitializer
{
    static typename ABI::Position
    init(const ThreadContext *tc)
    {
        return typename ABI::Position();
    }
};

template <typename ABI>
struct PositionInitializer<ABI, typename std::enable_if<
    std::is_constructible<typename ABI::Position, const ThreadContext *>::value
    >::type>
{
    static typename ABI::Position
    init(const ThreadContext *tc)
    {
        return typename ABI::Position(tc);
    }
};

template <typename ABI>
static typename ABI::Position
initializePosition(const ThreadContext *tc)
{
    return PositionInitializer<ABI>::init(tc);
}

/*
 * This struct template provides a default allocate() method in case the
 * Result or Argument template doesn't provide one. This is the default in
 * cases where the return or argument type doesn't affect where things are
 * stored.
 */
template <typename ABI, template <class ...> class Role,
          typename Type, typename Enabled=void>
struct Allocator
{
    static void
    allocate(ThreadContext *tc, typename ABI::Position &position)
    {}
};

/*
 * If the return or argument type isn't void and does affect where things
 * are stored, the ABI can implement an allocate() method for the various
 * argument and/or return types, and this specialization will call into it.
 */
template <typename ABI, template <class ...> class Role, typename Type>
struct Allocator<ABI, Role, Type, decltype((void)&Role<ABI, Type>::allocate)>
{
    static void
    allocate(ThreadContext *tc, typename ABI::Position &position)
    {
        Role<ABI, Type>::allocate(tc, position);
    }
};

template <typename ABI, typename Ret, typename Enabled=void>
static void
allocateResult(ThreadContext *tc, typename ABI::Position &position)
{
    Allocator<ABI, Result, Ret>::allocate(tc, position);
}

template <typename ABI>
static void
allocateArguments(ThreadContext *tc, typename ABI::Position &position)
{
    return;
}

template <typename ABI, typename NextArg, typename ...Args>
static void
allocateArguments(ThreadContext *tc, typename ABI::Position &position)
{
    Allocator<ABI, Argument, NextArg>::allocate(tc, position);
    allocateArguments<ABI, Args...>(tc, position);
}

template <typename ABI, typename Ret, typename ...Args>
static void
allocateSignature(ThreadContext *tc, typename ABI::Position &position)
{
    allocateResult<ABI, Ret>(tc, position);
    allocateArguments<ABI, Args...>(tc, position);
}

} // namespace GuestABI

#endif // __SIM_GUEST_ABI_LAYOUT_HH__