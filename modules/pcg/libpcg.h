//      Copyright (c) 2013 Intel Corporation.
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either expressed or implied.
// See the EULA for the specific language governing permissions and
// limitations under the EULA.
//

// static char cvs_id[] = "$Id: libpcg.h 247697 2014-01-21 22:32:40Z dlkreitz $";

//
// This file contains the declarations for the external interfaces to the
// standalone code generator.
//

#ifndef _INCLUDED_LIBPCG
#define _INCLUDED_LIBPCG

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <setjmp.h>

#if defined(__cplusplus)
extern "C" {
#endif

// Client instruction type.  A CGInst is the basic unit of operation in the
// code generator.  The operation performed by a CGInst is defined by its
// opcode, which is specified by name when a CGInst is created.  Each CGInst
// has zero or more operands (i.e. inputs) and zero or more definitions
// (i.e. outputs).  The number and types of operands & definitions as well as
// any possible side effects are defined by the semantics of the opcode.
// The CGInst also represents the value defined by the instruction, if
// applicable.  Instructions that need to reference the value defined by an
// instruction do so by referencing the CGInst.  (See the 'r' operand type to
// CGCreateNewInst.)  Multiple-definition instruction use a separate CGInst for
// each definition.  (See the 'a' operand type to CGCreateNewInst.)
//
typedef uint32_t CGInst;
const CGInst CGInstInvalid = 0;

// Client label type.  A CGLabel refers to a location in code.  Labels are used
// to specify branch targets.
//
typedef uint32_t CGLabel;
const CGLabel CGLabelInvalid = 0;

// Client insertion point type.
//
// The insertion point is the code location where a new instruction is inserted
// by the CGInst creation routines and the place where a label is bound during
// CGBindLabel.
//
// The insertion point is updated as instructions are created and labels are
// defined.  So when code is generated in a linear fashion, there is no need
// to worry about the insertion point.  It can be set once and then never
// touched again.
//
// The client can use the CGGetCurrentInsertionPoint and
// CGSetCurrentInsertionPoint methods to capture and change the insertion point
// if it is more convenient to create code in a non-linear fashion.
//
typedef uint32_t CGInsertionPoint;
const CGInsertionPoint CGInsertionPointInvalid = 0;

// Client address type.  A CGAddr is an x86 addressing expression.  The general
// form of an x86 addressing expression is this.
//
// address = 32-bit immediate + base + index * scale
//
// A CGAddr expression may also include a CGSymbol.  The address of the symbol
// might or might not be known at code generation time, but conceptually it is
// added to the rest of the address expression.
//
// CGAddr objects may be transient or non-transient.  Transient CGAddr objects
// are freed the first time they are used.  Non-transient CGAddr objects must
// be freed explicitly by the client. In many cases, addresses are created for
// the sole purpose of creating an instruction that references the address.
// For the convenience of these cases, created address are transient by
// default.
//
typedef uint32_t CGAddr;
const CGAddr CGAddrInvalid = 0;

// Client symbol type.  A CGSymbol is an abstraction of a location in memory.
// Symbols are often used to reference things whose addresses are not yet
// known, e.g. data objects that get placed by the linker.  CGSymbols, once
// created, are valid for an entire module.
//
typedef uint32_t CGSymbol;
const CGSymbol CGSymbolInvalid = 0;

// Client temporary value type.  Internally, the code generator's intermediate
// language is implemented as a use-def(UD) graph.  Clients may choose to
// manage the UD information themselves or have the code generator do it.
// Clients that choose to have the code generator compute the UD information
// will use CGTemps.  A CGTemp is a symbolic representation of a value.
// Instruction definitions can assign a value to a CGTemp, and instruction
// operands can reference the value stored in a CGTemp.  Clients communicate
// the CGTemp assigned by an instruction definition, if any.  And clients
// communicate the CGTemps referenced by an instruction, if any.  For each
// CGTemp reference, the code generator will compute the set of definitions of
// that CGTemp that can reach the reference and will restructure the UD graph
// accordingly.
//
typedef uint32_t CGTemp;

// Client relocation type specifier.  This enumeration is used to describe
// the relocation action for the client callback routine,
// CGAddRelocationToClient.  The relocation action is described in a comment
// before each relocation type.  In each case, target_addr gives the location
// in the code at which to apply the relocation action.  It is computed as
// routine_start_addr + code_offset, where routine_start_addr is address
// passed to the client as *start_addr during a successful call to
// CGGetBinaryCode and code_offset is the value passed to the client in the
// CGAddRelocationToClient callback.  symbol and addend are two properties of
// a relocation and are passed from PCG to the client via the
// CGAddRelocationToClient callback function.
//
typedef enum {
    CGRelocationTypeInvalid = 0,

    // *((uint32_t*)target_addr) = &symbol + addend
    CGRelocationType32,

    // *((uint64_t*)target_addr) = &symbol + addend
    CGRelocationType64,

    // *((uint32_t*)target_addr) = &symbol - target_addr + addend
    CGRelocationTypePC32
} CGRelocationType;

// IL creation routines
//
// Instructions consist of an operation and a list of operands.  Operations
// are specified by name.  We will document the supported operations and their
// semantics in the API.  The number and types of operands are specified by a
// descriptor string, and the operands themselves are specified as a variable
// argument list following the descriptor string.  The order of the operands in
// the argument list matches the characters in the descriptor string.  The
// currently supported characters are
//
// 'c': The operand is a condition for instructions such as conditional
//      branches and conditional selects.  The value of the operand is passed
//      using type const char *.  Supported condition names will be documented
//      in the API.  'c' may be placed at an arbitrary position in the list of
//      operands.
//
// 'B': The operand is a broadcast specifier for a vector instruction.
//      Supported broadcasts include "1to16", "1to8", "4to16", "4to8", and
//      "none".  If no broadcast is specified for an instruction that supports
//      a broadcast, the default is "none".  'B' may be placed at an arbitrary
//      position in the list of operands.
//
// 'U': The operand is an up conversion specifier for a vector instruction.
//      Supported up conversions include "float16", "srgb8", "uint8", "sint8",
//      "unorm8", "snorm8", "uint16", "sint16", "unorm16", "snorm16", "uint8i",
//      "sint8i", "uint16i", "sint16i", "unorm10A", "unorm10B", "unorm10C",
//      "unorm2D", "float11A", "float11B", "float10C", and "none".  If no
//      up conversion is specified for an instruction that supports an up
//      conversion, the default is "none".  'U' may be placed at an arbitrary
//      position in the list of operands.
//
// 'D': The operand is a down conversion specifier for a vector instruction.
//      Supported down conversions include "float16", "float16rz", "uint8",
//      "sint8", "unorm8", "snorm8", "uint16", "sint16", "unorm16", "snorm16",
//      "uint8i", "sint8i", "uint16i", "sint16i", and "none".  If no down
//      conversion is specified on an instruction that supports a down
//      conversion, the default is "none".  'D' may be placed at an arbitrary
//      position in the list of operands.
//
// 'Z': The operand is a swizzle specifier for a vector instruction.  Supported
//      swizzles include "cdab", "badc", "aaaa", "bbbb", "cccc", "dddd",
//      "dacb", and "none".  ("none" is equivalent to "dcba".)  If no swizzle
//      is specified for an instruction that supports a swizzle, the default is
//      "none".  'Z' may be placed at an arbitrary position in the list of
//      operands.
//
// 'z': There is no operand.  This is only applicable to write-masked vector
//      instructions on AVX512 platforms, and indicates the instruction should
//      use zero masking.  'z' may be placed at an arbitrary position in the
//      descriptor string.
//
// 'R': The operand is an explicit rounding mode specifier for instructions
//      that support an explicit rounding mode.  Supported rounding mode
//      specifiers include "rn" (round to nearest), "rd" (round toward negative
//      infinity), "ru" (round toward positive infinity), "rz" (round toward
//      zero), and "none" (hardware default rounding mode, usually taken from a
//      control register).  If no rounding mode is specified for an instruction
//      that supports an explicit rounding mode, the default is "none".  'R'
//      may be placed at an arbitrary position in the list of operands.
//
// 'S': There is no operand.  Indicates the Suppress All Exceptions (SAE)
//      feature should be enabled on the instruction.  'S' may be placed
//      at an arbitrary position in the descriptor string.
//
// 'H': The operand is a memory hint specifier for a load/store instruction
//      that supports such a hint.  Supported memory hints include "nt", "eh",
//      and "none".  If no hint is specified for an instruction that supports a
//      memory hint, the default value is none.  'H' may be placed at an
//      arbitrary position in the list of operands.
//
// 'i': The operand is a 32-bit integer.  The value of the operand is passed
//      using type int32_t.
//
// 'j': The operand is a 64-bit integer.  The value of the operand is passed
//      using type int64_t.
//
// 'r': The operand is a reference to another instruction.  The value of the
//      operand is passed using type CGInst.
//
// 'm': The operand is a memory reference.  The value of the operand is passed
//      using three arguments: a CGAddr to specify the address, a uint32_t to
//      specify the size, and a void* to specify the handle.  The handle is an
//      opaque type that is passed back to the client in the memory
//      disambiguation callback routine, CGGetProbabilityOfOverlapFromClient.
//
// 'v': The same as 'm' except that the memory reference is considered
//      volatile.
//
// 'd': The operand is a pure address.  The value of the operand is passed as a
//      CGAddr.  'd' operands differ from 'm' operands in that they do not
//      actually dereference the address, so the size and handle are not
//      applicable.  'd' operands are used in instructions such as lea.
//
// 'n': The operand is a symbol.  The value of the symbol is currently passed
//      using type const char *, but this needs to change.  We need a more
//      generic concept of a symbol.  (The current implementation was a hack
//      to get hello, world working.)
//
// 'l': The operand is a list of instruction references.  The value of the
//      operand is passed using an of type CGInst, terminated by CGInstInvalid.
//
// 'b': The operand is a label, typically used for branch instructions.  The
//      value of the operand is passed using type CGLabel.
//
// 'a': This descriptor indicates that the instruction being created is not
//      really a new instruction but rather a sub-definition of an existing
//      instruction.  The operand specifies this existing "root" instruction
//      using type CGInst.
//
// 'I': The operand is a const char * that specifies the name of the intrinsic
//      function.  This operand type is only allowed when creating a call
//      instruction.
//
// 'P': The operand consists of two const char * arguments that define an
//      attribute=value pair for an intrinsic math function.  The attribute
//      name is the first argument, and the value name is the second.  This
//      operand type is only valid when creating an intrinsic function.  Legal
//      strings for the attribute and value names are as follows:
//
//          accuracy-bits-32
//          accuracy-bits-64
//              These attributes specify the required number of accurate bits
//              in the mantissa of the result for single and double precision,
//              respectively.  The value is specified as a double.
//
//          max-error
//              This attribute is an alternative way to specify the required
//              accuracy for an intrinsic math function.  The value is
//              specified as a double and gives the required accuracy in ulps
//              (units in the last place).  Only one of accuracy-bits-32|64 and
//              max-error should be specified for each function.  Otherwise,
//              the behavior is undefined.
//
//          domain-exclusion
//              This attribute specifies what special input values, if any,
//              may be ignored in the implementation of the math function.
//              (That is, it species the input values for which the
//              implementation may give garbage results.)  The value is
//              specified as an integer, which is interpreted as a bit mask.
//              The individual bits in the mask mean the following:
//                  0x1:  extreme values (defined on a per function basic)
//                  0x2:  NaNs
//                  0x4:  infinities
//                  0x8:  denormals
//                  0x10: zeros
//
// 'h': The operand is a void* that specifies the memory handle for an
//      intrinsic call.  This operand type is only valid when creating an
//      intrinsic function.  The specified handle is applied to any memory
//      references that get created as part of the intrinsic expansion.  If the
//      intrinsic expands to multiple memory references, they will all get this
//      same handle (which might not be as precise as you would like).  The
//      handle is an opaque type that is passed back to the client in the
//      disambiguation callback routine, CGGetProbabilityOfOverlapFromClient.
//
// There are several IL creation routines.  CGCreateNewInst creates and returns
// a new instruction.  CGAllocateInst allocates and returns a new instruction
// but does not actually create it.  The returned instruction may be referenced
// by other instructions before it is actually created.  CGCreateAllocatedInst
// creates an instruction that has already been allocated by CGAllocateInst.
//
extern CGInst CGCreateNewInst(const char *name, const char *op_descr, ...);
extern CGInst CGAllocateInst(void);
extern void CGCreateAllocatedInst(CGInst pil_id, const char *name,
                                  const char *op_descr, ...);

// CGCreateEntryInst returns an entry instruction and sets the insertion point
// to insert after the returned instruction [as if the client had called
// CGSetInsertionPointAfter(inst) on the returned instruction].
//
extern CGInst CGCreateEntryInst(void);

// CGGetStackPointerDef returns an instruction that can be used to reference
// the stack pointer.  The instruction can be used to reference the stack
// pointer at any point in the program.  As a result, the actual value defined
// by the instruction will vary depending on where it is referenced.
//
extern CGInst CGGetStackPointerDef(void);


// Client source position type.
typedef void* CGSrcPosHandle;

// CGSetSourcePosition defines the current source position to be associated
// with any subsequently created instructions.  It may be NULL, in which
// case the CG stops associating source position with new instructions.
//
extern void CGSetSourcePosition(CGSrcPosHandle sp);

// CGSetRreg defines the physical result register for the specified
// instruction.  The register is specified by name.
//
extern void CGSetRreg(CGInst pil, const char *reg_name);

// CGGetMaxCodeAlignment returns the maximum code alignment that pcg will ever
// require.  The routine does not consider client-requested alignment, which
// could be arbitrarily strict.
//
extern uint32_t CGGetMaxCodeAlignment(void);

// The CGGetBinaryCode routine is intended to be called after pcg generates
// code for a routine, i.e. after the call to CGCompileRoutine.
//
// CGGetBinaryCode requests that pcg write the binary code for a routine into
// a buffer provided by the client.  The start address and length of the buffer
// are given by code_ptr and len, respectively.  The align parameter does not
// specify the alignment of code_ptr but rather the highest base for which the
// alignment of code_ptr can be trusted.  In other words, pcg can assume that
// (code_ptr % align) is a fixed quantity.
//
// Based on the requirements of the routine, pcg writes the start address,
// end address (address of the last byte of routine code), and required
// alignment for the routine to *start_addr, *end_addr, and *required_align,
// respectively.  If the code_ptr buffer has sufficient size and alignment, the
// binary code is written to the buffer and the routine returns a non-zero
// value.  If the buffer is too small or insufficiently aligned, no code is
// written, and the routine returns 0.
//
// When CGGetBinaryCode fails (returns 0), the client can determine the size
// and alignment requirements from *start_addr, *end_addr, and *required_align,
// allocate an adequate buffer, and call the routine again.
//
extern int CGGetBinaryCode(uint8_t *code_ptr, size_t len, uint32_t align,
                           uint8_t **start_addr, uint8_t **end_addr,
                           uint32_t *required_align);

// The CGCreateModule function is called to create a new module in PCG and
// prepare it for the creation and compilation of routines.
//
extern void CGCreateModule(const void *client_module_handle);

// The CGEndModule function is called to do module level cleanup.  This cleanup
// includes writing to the assembly file, if one exists.  The client must
// ensure that the assembly file handle remains open until this routine is
// called.
//
// In the current implementation, CGEndModule is called automatically if
// CGCreateModule is called without first calling CGEndModule for the previous
// module.
//
extern void CGEndModule(const void *client_module_handle);

// The CGCreateRoutine function is called to create a new routine in PCG and
// prepare it for Pil creation.
//
extern void CGCreateRoutine(const void *client_routine_handle);

// CGEndRoutine resets all routine level state for PCG.  In the current
// implementation, CGEndRoutine is called automatically if CGCreateRoutine is
// called without first calling CGEndRoutine for the previous routine.
//
extern void CGEndRoutine(const void *client_routine_handle);

// The CGCompileRoutine is called after all the PCGil has been created for a
// routine.  It does all the work to optimize and generate the final binary
// and/or assembly code.
//
extern void CGCompileRoutine(const void *client_routine_handle);

// Temp management routines
//
// CGAddTempDef communicates to PCG that the instruction, def, defines temp.
// CGGetTempUseInst gets an instruction that may be used in the instruction
// creation interfaces to reference temp.  The instruction returned by
// CGGetTempUseInst is a dummy instruction that should be used only as an
// operand in the instruction creation interfaces.
// CGTempIsDefined is a convenience interface to query whether PCG has "seen"
// temp yet.  It returns 1 if either CGGetTempUseInst or CGAddTempDef has been
// called for temp, and it returns 0 otherwise.
//
extern CGInst CGGetTempUseInst(CGTemp temp);
extern void CGAddTempDef(CGTemp temp, CGInst def);
extern int CGTempIsDefined(CGTemp temp);

// Insertion point management routines
//

// CGSetInsertionPointAfter sets the insertion point after the specified
// instruction.  After calling this routine, when new instructions are created,
// the insertion point is updated to insert after the newly created inst.  The
// effect is that instructions come out in linear order after inst.
//
extern void CGSetInsertionPointAfter(CGInst inst);

// CGSetInsertionPointBefore sets the insertion point before the specified
// instruction.  After calling this routine, when new instructions are created,
// they will always be inserted before inst.  The insertion point is not
// updated until a later to call to one of the CGSetInsertionPoint routines
// modifies it.  The effect is that instructions come out in linear order
// before inst.
//
extern void CGSetInsertionPointBefore(CGInst inst);

// Get and set the current insertion point
extern CGInsertionPoint CGGetCurrentInsertionPoint(void);
extern void CGSetCurrentInsertionPoint(CGInsertionPoint insertion_point);

// Free the specified insertion point
extern void CGFreeInsertionPoint(CGInsertionPoint insertion_point);

// Address management routines
//

// CGCreateAddr creates an address from its component parts based on legal
// Intel architecture addressing modes.  The returned CGAddr is transient.
//
extern CGAddr CGCreateAddr(CGInst base, CGInst index, uint32_t scale,
                           CGSymbol ltbase, int32_t offset);

// Create an address that differs from addr by offset bytes.  The returned
// CGAddr is transient.  If addr is transient, it is freed by this routine.
//
extern CGAddr CGAddrAdjustOffset(CGAddr addr, int32_t offset);

// Create an address that differs from addr by index * scale.  addr must not
// already use a scaled index.  The returned CGAddr is transient.  If addr is
// transient, it is freed by this routine.
//
extern CGAddr CGAddrAddScaledIndex(CGAddr addr, CGInst index, uint32_t scale);

// CGAddrRetain marks a CGAddr object non-transient.  CGAddrFree explicitly
// frees a CGAddr object.  The CGAddr passed to CGAddrFree may be either
// transient or non-transient.
//
extern void CGAddrRetain(CGAddr addr);
extern void CGAddrFree(CGAddr addr);

// Routine for accessing formal parameters and the return address
//
// This routine creates an address that references the stack location
// containing the return address.  The resulting address can be used to access
// the return address, or it can be adjusted (usually via CGAddrAdjustOffset)
// to access a formal parameter.
//
extern CGAddr CGGetReturnAddressAddr(void);

// Local variable allocation routine
//
// Allocate a local variable of the specified size and alignment.  align must
// be a power of two.  (The upper bound on the alignment is TBD.)
// This routine returns a CGAddr that gives the base address of the new
// variable.  The returned CGAddr is transient.
//
extern CGAddr CGAllocLocal(uint32_t size, uint32_t align);

// Symbol management routines
//

// CGCreateSymbol creates a client symbol.
//
// The symbol_attributes string specifies zero or more attribute characters
// describing the symbol.  The currently supported attribute characters are
//
// 'c': The symbol references constant memory.  The code generator may assume
//      that all loads relative to the symbol are constant for the duration of
//      the routine.
//
// 'l': The symbol is local to a routine. The code generator may remove any
//      record of the symbol at the end of routine.  Local symbols may only be
//      created after calling CGCreateRoutine and before calling CGEndRoutine.
//
// 'p': References to the symbol may be patched in an unsynchronized cross
//      modifying code environment.  (Cross modifying code means that one
//      processor modifies code that another processor is executing.)
//
extern CGSymbol CGCreateSymbol(const char *symbol_attributes);

// CGResolveSymbolReferences requests that PCG resolve the symbol references
// in the generated code for the routine given by client_routine_handle.  In
// order to do so, PCG queries the client for absolute symbol addresses using
// the CGGetSymbolAddressFromClient callback function.
//
extern void CGResolveSymbolReferences(const void *client_routine_handle,
                                      uint8_t *code_ptr);

// CGSetSymbolConstantValue requests that PCG associate a constant value with
// the specified symbol.  If a subsequent instruction is created which
// references an address relative to this symbol PCG may use the value to
// optimize certain operations.
//
extern void CGSetSymbolConstantValue(CGSymbol symbol,
                                     uint8_t *buffer,
                                     uint32_t size);

// Label management routines
//
// CGLabel is the mechanism that enables the client to introduce control flow
// into a routine.  CGCreateLabel creates a new label that may be referenced
// immediately in instructions like conditional branches that take a label as
// and operand.  CGBindLabel binds a label to the current insertion point.
//
extern CGLabel CGCreateLabel(void);
extern void CGBindLabel(CGLabel label);

// CGAddIndirectBranchTarget tells the CG that the specified label is a
// possible target of the specified instruction, which must be an indirect
// branch.
//
extern void CGAddIndirectBranchTarget(CGInst, CGLabel);

// CGGetLabelNameAndOffset queries PCG for the assembly name and code offset
// of the specified label.  The code offset is relative to the start address
// of the routine containing the label.  This routine must be called after
// CGCompileRoutine has been called for the routine containing the label.  And
// it may be called any time before CGEndModule of the containing module.
// This routine may only be called on labels that were also passed
// as arguments to CGBindLabel.
// The int64_t* argument may be NULL in which case, only the label name is
// returned.
//
extern const char *CGGetLabelNameAndOffset(CGLabel, int64_t *);

// CGSetAsmOutputFile tells the CG to output assembly code into the FILE
// passed in as the parameter. If as_file is NON-NULL, then as_file must be
// an open and writable file at the time of the call.  Writes to this file
// will occur during calls to CGCompileRoutine and CGEndModule, so the file
// must stay open until all calls to CGCompileRoutine and CGEndModule have
// returned. To turn off assembly production, you can call this with a NULL
// file pointer.
//
extern void CGSetAsmOutputFile(FILE *as_file);

// CGSetTraceOutputFile tells the CG to output a trace of the CG interface
// calls into the FILE passed in as the parameter.  This feature is designed
// to create reproducers for debugging code generation problems.  If trace_file
// is NON-NULL, then trace_file must be an open and writable file at the time
// of the call.  Writes to this file will occur during most interface calls.
// Tracing can be disabled at any time by calling CGSetTraceOutputFile(NULL).
//
extern void CGSetTraceOutputFile(FILE *trace_file);

// CGSetSetjmpContext and CGGetSetjmpContext provide a failure recovery
// mechanism for clients of the CG.  When the CG encounters an internal or
// fatal error, it will call longjmp with the most recently defined context and
// an argument of 1.  It will also set the setjmp context to NULL so that
// subsequent errors do not mistakenly call longjmp to the same setjmp context
// twice.  If no setjmp context has been defined or if the most recently
// defined context is NULL, the CG will instead call exit.  Clients have some
// flexibility in how they construct their error handling system, but typical
// usage will look similar to this:
//
//     jmp_buf client_context;
//     jmp_buf *prev_context = CGGetSetjmpContext();
//     if (setjmp(client_context) == 0) {
//         CGSetSetjmpContext(&client_context);
//         <CG API calls>
//     }
//     else {
//         <client failure recovery code>
//     }
//     CGSetSetjmpContext(prev_context);
//
extern void CGSetSetjmpContext(jmp_buf *client_context);
extern jmp_buf *CGGetSetjmpContext(void);

// CGConfigureRoutine enables the client to specify zero or more configuration
// parameters.  Each configuration parameter is specified by name and is
// followed by a second string argument specifying the parameter's value.
// The client may specify an arbitrary number of <parameter, value> pairs in
// the argument list.  The list must be terminated by a NULL string for the
// next parameter name.
//
// CGConfigureRoutine must be called after a routine has been created via
// CGCreateRoutine and before the routine is compiled via CGCompileRoutine.
//
// The remainder of this comment lists the configuration parameters that are
// currently supported.  It also lists the expected format for the parameter's
// value.  Currently, there are three different formats.  Boolean parameters
// can take a value of "on", "off", "true", or "false".  Integer parameters
// take a 64-bit integer in string format.  And string parameters take a
// string value.  Any expectations about the string value are noted in the
// description of the configuration parameter.
//
// Parameter                                     Value Format   Default
// --------------------------------------------------------------------
//
// debug_level                                   integer        0
//     When non-zero, causes debugging output to be printed to stderr.  To get
//     a PCGil dump after every major compilation phase, use a value of "1".
//
// target_arch                                   string         "sse2"
//     Define the target architecture by processor.  This parameter defines the
//     set of instructions that PCG may automatically generate.  It also
//     defines the processor for which the code will be tuned.  Supported
//     values include "ia32", "sse", "sse2", "sse3", "ssse3", "sse4.1",
//     "sse4.2", "ssse3_atom", "avx", "core_avx_i", and "core_avx2".
//
// fp_value_safety                               string         "fast"
//     Define the floating-point value safety level.  Supported values include
//     "safe" and "fast".  Under "safe", PCG will not make any transformations
//     that could change the result of an FP calculation.  Under "fast", PCG
//     may perform some transformations that could have a minor effect on the
//     result of an FP calculation.  For example, it might transform
//     x + 0.0 --> x.  If x happens to be -0.0, the result will be -0.0 instead
//     of the IEEE correct +0.0.
//
// fp_contract                                   boolean        true
//     Enable or disable floating-point contractions, typically fused
//     multiply-add (FMA) and related instructions.  When contractions are
//     enabled, the code generator may combine separate multiply and add
//     instructions into a single FMA instruction with no intermediate
//     rounding.  This transformation makes the result more accurate, but since
//     the compiler might or might not choose to make the transformation, the
//     result is less predictable.
//
// fenv_access                                   boolean        false
//     Enable or disable floating-point environment access.  When fenv_access
//     is disabled, the code generator may assume the default floating-point
//     environment, which typically means that all FP exceptions are masked and
//     that the rounding mode is round-to-nearest.  It also means the code
//     generator may assume that the program does not read the status bits.
//     From a practical perspective, this means the code generator can perform
//     optimizations like dead code elimination, constant folding, and various
//     forms of redundancy elimination.  When fenv_access is enabled, the code
//     generator is careful to honor the floating-point environment settings
//     and to preserve the status bit settings implied by the code.
//
// complex_limited_range                         boolean        false
//     Enable or disable complex_limited_range.  The standard mathematical
//     expressions for complex arithmetic can overflow or underflow when
//     evaluated using a floating-point representation with finite precision
//     and range, even when the result is within representable range.  When
//     complex_limited_range is enabled, the code generator may evaluate
//     complex arithmetic using the standard mathematical expressions.  When it
//     is disabled, the code generator will be careful to use calculations that
//     preserve the full range.  The code to do so might be slower than the
//     code generated under complex_limited_range.
//
// flush_to_zero                                 boolean        true
//     Enable or disable flush to zero (FTZ) and denormals are zero (DAZ)
//     modes.  When flush_to_zero is enabled, the code generator may generate
//     code that flushes denormal results to zero or treats denormal inputs as
//     zero.  Regardless of whether flush_to_zero mode is enabled or disabled,
//     the code generator is never required to flush denormal results to zero
//     or to treat denormal inputs as zero.
//
// fp_exception_semantics                        boolean        false
//     Enable or disable precise floating-point exceptions.  When
//     fp_exception_semantics is true, the code generator will produce code
//     that preserves all exceptions implied by the input code.  It will also
//     ensure that any exception is raised at the point in the program where
//     the exception occurred.  Practically speaking, this means that the code
//     generator will emit an fwait instruction after any x87 instruction that
//     might raise an exception.
//
// prec_div                                      boolean        false
//     Enable or disable precise floating-point division.  When prec_div is
//     true, the code generator will generate code that gives IEEE compliant
//     results for floating-point division.  When prec_div is false, the
//     code generator may use faster but less accurate divide implementations.
//     It might also evaluate A / B as A * (1 / B) or vice versa.
//
// prec_sqrt                                     boolean        false
//     Enable or disable precise floating-point square root.  When prec_sqrt is
//     true, the code generator will generate code that gives IEEE compliant
//     results for floating-point sqrt.  When prec_sqrt is false, the code
//     generator may use faster but less accurate sqrt implementations.
//
// fp_speculation                                boolean        true
//     Enable or disable floating-point speculation.  When fp_speculation is
//     true, the compiler might move a floating-point expression such that the
//     generated code evaluates the expression in cases where the original
//     program did not.  This can be useful for performance, e.g. by hoisting
//     a loop invariant expression out of a loop, but it can also raise
//     spurious exceptions.  It can be useful to disable fp_speculation in
//     programs that unmask exceptions the programmer wants to consider as
//     errors (e.g. invalid and divide-by-zero) but that otherwise do not
//     access the floating-point environment.
//
// pic                                           boolean        false
//     When "true", causes symbol references to be position independent.  This
//     means that PCG will use IP-relative addressing when accessing symbols
//     whose locations are known relative to the generated code.  (PCG assumes
//     that relative symbols addresses are known unless the callback function
//     CGSymbolsNeedsLargeModelFixup returns true.)
//
// eliminate_frame_pointer                       boolean        true
//     When "true", PCG will attempt to eliminate the frame pointer and use
//     the frame pointer register as a general purpose register.  Clients may
//     choose to disable this optimization if they rely on EBP/RBP chaining
//     to compute stack backtraces.
//
// stack_alignment_compatibility                 integer        2
//     IA32 Specific.
//     Changes the stack alignment mode.
//     By default, we assume we have 16B (before return address) alignment
//     coming into all routines, and we try to maintain 16B alignment going
//     out from all calls.
//     This is mode "2", and how GCC and ICC operate, by default.
//     Note that, for Windows, the default is "0".
//
//     This interface is used to change the default behavior.
//     "0" is used to completely remove all alignment, and only align the stack
//     dynamically on entry to routines that need it (using extra registers and
//     instructions).
//     "1" is used to do the same as "0", but also try to maintain 16B
//     alignment when we call outside of the routine, just in case we're
//     calling someone that expects 16B aligned frame on entry, due to
//     operation in mode "2".
//
//     0 = disable all 16B alignment. Align stack dynamically if we need it.
//     1 = Align stack dynamically if we need it, but try to maintain 16B
//         alignment.
//     2 = Assume 16B alignment everywhere, and never do dynamic stack
//         alignment.
//
// expand_32b_idiv_irem                          boolean        variable
//     Enable or disable the expansion of 32bit idiv and irem instructions
//     into a sequence of if-then-else that checks to see
//     if we can use a much faster (on current Atom architectures)
//     8bit divide, instead of a slow 32bit divide. By default, this
//     optimization is on when targeting
//     "atom_ssse3", and "atom_sse4.2", and off otherwise.
//
// opt_level                                     integer        2
//     Sets PCG optimization level to value. Acceptable value range: 0 to 3.
//
// use_pcg_linking                               boolean        false
//     Indicates whether the client intends to use CGResolveSymbolReferences
//     to perform linking on the external symbol references in the routine.
//     If false, it is illegal to call CGResolveSymbolReferences for the
//     routine.
//


extern void CGConfigureRoutine(const char *config_parameter, ...);

// CGDiagnosticLevel defines the severity of diagnostics reported by the
// CGDiagnosticMessage callback routine.
//
// User-level diagnostics inform the programmer about illegal or suspect input.
// There are four categories of user-level diagnostics: remarks, warnings,
// errors, and fatal errors.  The difference between an error and a fatal error
// is that compilation is allowed to continue for a non-fatal error.
// Additional diagnostic conditions are detected and reported.  When a fatal
// error is encountered, compilation stops immediately.
//
// Internal errors are reported when the code generator fails an assertion and
// cannot continue.
//
typedef enum {
    CGDiagnosticLevelInvalid = 0,
    CGDiagnosticLevelRemark,
    CGDiagnosticLevelWarning,
    CGDiagnosticLevelError,
    CGDiagnosticLevelFatal,
    CGDiagnosticLevelInternal
} CGDiagnosticLevel;

// CGRegisterCallbackRoutine provides PCG with a function pointer to one of the
// client callback routines.  The code generator calls these in order to get
// information from the client or to request that the client perform some
// action.
//
// Callback routines supported by this interface include the following.
//
// callback_name: "CGDiagnosticMessage"
// Prototype: void CGDiagnosticMessage(CGDiagnosticLevel severity,
//                                     const char *file, uint32_t line,
//                                     uint32_t col, const char *error_msg);
// Description: PCG calls CGDiagnosticMessage to pass diagnostic information
//     back to the client.  This can be for user-level errors, warnings, and
//     remarks or for internal errors, which are typically assertion failures.
//     PCG provides the diagnostic severity, source position information
//     (file, line, col), and a descriptive error message.  For fatal errors
//     and internal errors, after the client returns from CGDiagnosticMessage,
//     PCG will raise an exception by calling longjmp if the client has set a
//     setjmp context using CGSetSetjmpContext.  Otherwise, it will exit.
// Default behavior: If the client does not provide a CGDiagnosticMessage
//     callback routine, then when a diagnostic needs to be generated, PCG will
//     print a message of its own choosing to stderr.
//
//
// callback_name: "CGSetBinarySourcePosition"
// Prototype: void CGSetBinarySourcePosition(uint64_t code_offset,
//                                           CGSrcPosHandle sp);
// Description: PCG invokes this callback to tell the client that the
//     instruction at offset "code_offset" from the start of the routine is
//     associated with the specified source position.
//     PCG invokes this routine in ascending "code_offset" order for every
//     change in source position.
//
// Default behavior: If the client does not provide a CGSetBinarySourcePosition
//     callback routine, then no source position information emitted
//     for client. Assembler path is not affected whether it registered or not.
//     If the callback is registered PCG emits source position information
//     regardless whether it was set to valid source position or not.
//     That means that client should expect NULL value for source position.
//
//
// callback_name: "CGGetSourcePosition"
// Prototype: const char* CGGetSourcePositionFunction(CGSrcPosHandle sp,
//                                                    int32_t *line,
//                                                    int32_t *col);
// Description: PCG invokes this callback to translate non-null CGSrcPosHandle
//     source position data into source file, line number and column number
//     representation. File name is provided as return value while the other
//     two via pointer arguments.
//
// Default behavior: If the client does not provide a CGGetSourcePosition
//     callback routine but sets valid (non-null) source position information
//     using CGSetSourcePosition interface routine then attempt to get source
//     file name, or line number, or column number for that source position
//     results in diagnostics error message. PCG normally does not call the
//     callback with NULL source position.
//
//
// callback_name: "CGGetMemConstSymbolFromClient"
// Prototype: CGSymbol CGGetMemConstSymbolFromClient(
//                         const void *client_routine_handle,
//                         uint8_t *value,
//                         size_t length,
//                         uint32_t align);
// Description: CGGetMemConstSymbolFromClient requests that the client allocate
//     memory to hold a constant value and then create a CGSymbol that the code
//     generator can use to reference that memory.  The memory must be at least
//     "length" bytes and have at least "align" alignment.  The client must
//     copy the first "length" bytes from "value" to the newly allocated
//     memory.
//
// Default behavior: If the client does not provide a
//     CGGetMemConstSymbolFromClient callback routine, the code generator will
//     issue a fatal diagnostic if it ever needs to make this callback.  All
//     clients are encouraged to implement CGGetMemConstSymbolFromClient.
//

extern void CGRegisterCallbackRoutine(const char *callback_name, void *fnptr);

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//                 Client callback routines follow
//
// These routines must all be defined by the client.  The code generator calls
// these in order to get information from the client or to request that the
// client perform some action.

// CGGetRoutineNameFromClient requests that the client provide the code
// generator with the name of the specified function.  The code generator
// passes the same client_routine_handle that was passed to it via
// CGCreateRoutine and CGCompileRoutine.
//
extern const char *CGGetRoutineNameFromClient(
    const void *client_routine_handle
);

// CGGetMemConstSymbolFromClient requests that the client allocate memory to
// hold a constant value and then create a CGSymbol that the code generator can
// use to reference that memory.  The memory must be at least "length" bytes
// and have at least "align" alignment.  The client must copy the first
// "length" bytes from "value" to the newly allocated memory.
//
extern CGSymbol CGGetMemConstSymbolFromClient(uint8_t *value, size_t length,
                                              uint32_t align);

// CGSymbolNeedsLargeModelFixup asks the client whether the specified symbol
// might have an arbitrary 64-bit address.  If so, the client must return a
// non-zero value.  If the symbol is known to reside in the lower 2GB of
// the address space or if the symbol is known to be located within 2GB of
// the generated code in PIC mode, then the client may return 0.
//
extern int CGSymbolNeedsLargeModelFixup(CGSymbol symbol);

// CGGetSymbolAddressFromClient requests that the client provide the absolute
// address of the specified symbol.  PCG uses this information to process
// relocations during calls to CGResolveSymbolReferences.  The return value
// is defined as uint64_t to accommodate both 32-bit and 64-bit targets.
//
extern uint64_t CGGetSymbolAddressFromClient(CGSymbol symbol);

// CGGetSymbolForNameFromClient requests that the client provide a CGSymbol
// that PCG can use to reference an object level symbol of the specified name.
// This callback function is typically used for library symbols resulting from
// intrinsic function expansions.
//
extern CGSymbol CGGetSymbolForNameFromClient(const char *symbol_name);

// CGGetSymbolNameFromClient requests that the client provide the name of the
// specified symbol.
//
extern const char *CGGetSymbolNameFromClient(CGSymbol symbol);

// CGGetProbabilityOfOverlapFromClient requests that the client provide
// disambiguation information about the memory references identified by
// handle1 and handle2.  (handle1 and handle2 are the handles that were passed
// to CGCreateNewInst for 'm' operands.)
//
// The client must return an integer in the range [0, 100].  A return value of
// 0 is a guarantee from the client that the memory references do not overlap.
// A return value of 100 is a guarantee from the client that the memory
// references do overlap.  Any other value is the client's best guess for the
// probability that the memory references overlap.
//
extern uint32_t CGGetProbabilityOfOverlapFromClient(void *handle1,
                                                    void *handle2);

// CGAddRelocationToClient passes relocation information back to the client.
// This routine is called as many times as necessary during the call to
// CGCompileRoutine.  The client_routine_handle identifies the routine for the
// client.  It is the same handle that was passed to PCG in the call to
// CGCreateRoutine.  code_offset gives the location in the code at which to
// apply the relocation action.  It is an offset from the start of the
// function.  symbol gives the CGSymbol to which the relocation must be made.
// relocation_type gives the type of relocation.  addend specifies a constant
// addend used to compute the value to be stored in the relocatable field.
//
extern void CGAddRelocationToClient(void *client_routine_handle,
                                    uint64_t code_offset,
                                    CGSymbol symbol,
                                    CGRelocationType relocation_type,
                                    int64_t addend);

#if defined(__cplusplus)
}
#endif

#endif // _INCLUDED_LIBPCG
