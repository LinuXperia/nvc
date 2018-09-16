//
//  Copyright (C) 2018  Nick Gasson
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "jit.h"
#include "jit-priv.h"

typedef enum {
   __W0 = 0,
} arm64_reg_t;

static void arm64_ret(jit_state_t *state)
{
   __(0xc0, 0x03, 0x5f, 0xd6);
}

static void arm64_mov_reg_imm(jit_state_t *state, arm64_reg_t dest,
                              int64_t imm)
{
   __(0x40, 0x05, 0x80, 0x52);
}

void jit_prologue(jit_state_t *state)
{
}

void jit_epilogue(jit_state_t *state)
{
}

void jit_patch_jump(jit_patch_t patch, uint8_t *target)
{
}

static void jit_op_const(jit_state_t *state, int op)
{
   jit_vcode_reg_t *r = jit_get_vcode_reg(state, vcode_get_result(op));
   r->state = JIT_CONST;
   r->value = vcode_get_value(op);
}

static void jit_op_return(jit_state_t *state, int op)
{
   if (vcode_count_args(op) > 0) {
      vcode_reg_t result_reg = vcode_get_arg(op, 0);
      jit_vcode_reg_t *r = jit_get_vcode_reg(state, result_reg);

      switch (r->state) {
         //case JIT_REGISTER:
         // x86_mov_reg_reg(state, __EAX, r->reg_name);
         //break;
      case JIT_CONST:
         arm64_mov_reg_imm(state, __W0, r->value);
         break;
         //case JIT_STACK:
         //x86_mov_reg_mem_relative(state, __EAX, __EBP,
         //                         r->stack_offset, r->size);
         //break;
      default:
         jit_abort(state, op, "cannot return r%d", result_reg);
      }
   }

   jit_epilogue(state);
   arm64_ret(state);
}

void jit_op(jit_state_t *state, int op)
{
   vcode_set_jit_addr(op, (uintptr_t)state->code_wptr);

   switch (vcode_get_op(op)) {
   case VCODE_OP_CONST:
      jit_op_const(state, op);
      break;
   case VCODE_OP_RETURN:
      jit_op_return(state, op);
      break;
#if 0
   case VCODE_OP_ADDI:
      jit_op_addi(state, op);
      break;
   case VCODE_OP_ADD:
      jit_op_add(state, op);
      break;
   case VCODE_OP_MUL:
      jit_op_mul(state, op);
      break;
   case VCODE_OP_ALLOCA:
      jit_op_alloca(state, op);
      break;
   case VCODE_OP_STORE_INDIRECT:
      jit_op_store_indirect(state, op);
      break;
   case VCODE_OP_LOAD_INDIRECT:
      jit_op_load_indirect(state, op);
      break;
   case VCODE_OP_JUMP:
      jit_op_jump(state, op);
      break;
   case VCODE_OP_CMP:
      jit_op_cmp(state, op);
      break;
   case VCODE_OP_COND:
      jit_op_cond(state, op);
      break;
#endif
   case VCODE_OP_COMMENT:
   case VCODE_OP_DEBUG_INFO:
      return;
#if 0
   case VCODE_OP_STORE:
      jit_op_store(state, op);
      break;
   case VCODE_OP_LOAD:
      jit_op_load(state, op);
      break;
   case VCODE_OP_BOUNDS:
      jit_op_bounds(state, op);
      break;
   case VCODE_OP_DYNAMIC_BOUNDS:
      jit_op_dynamic_bounds(state, op);
      break;
   case VCODE_OP_UNWRAP:
      jit_op_unwrap(state, op);
      break;
   case VCODE_OP_UARRAY_DIR:
      jit_op_uarray_dir(state, op);
      break;
   case VCODE_OP_UARRAY_LEFT:
      jit_op_uarray_left(state, op);
      break;
   case VCODE_OP_UARRAY_RIGHT:
      jit_op_uarray_right(state, op);
      break;
   case VCODE_OP_SELECT:
      jit_op_select(state, op);
      break;
   case VCODE_OP_CAST:
      jit_op_cast(state, op);
      break;
   case VCODE_OP_SUB:
      jit_op_sub(state, op);
      break;
   case VCODE_OP_RANGE_NULL:
      jit_op_range_null(state, op);
      break;
#endif
   default:
      jit_abort(state, op, "cannot JIT op %s",
                vcode_op_string(vcode_get_op(op)));
   }
}

void jit_bind_params(jit_state_t *state)
{
   const int nparams = vcode_count_params();
   for (int i = 0; i < nparams; i++) {
      jit_vcode_reg_t *r = jit_get_vcode_reg(state, vcode_param_reg(i));

      switch (vtype_kind(vcode_param_type(i))) {
      default:
         jit_abort(state, -1, "cannot handle parameters with type %d",
                   vtype_kind(vcode_param_type(i)));
      }
   }
}

void jit_reset(jit_state_t *state)
{
}

void jit_signal_handler(int signum, void *extra)
{
#if 0
   ucontext_t *uc = (ucontext_t*)extra;
   uintptr_t rip = uc->uc_mcontext.gregs[REG_RIP];

   if (signum == SIGTRAP)
      rip--;

   jit_state_t *jc = jit_find_in_cache((void *)rip);
   if (jc == NULL)
      return;

   vcode_select_unit(jc->unit);

   int mark_op = -1;
   const int nblocks = vcode_count_blocks();
   for (int i = 0; i < nblocks; i++) {
      vcode_select_block(i);

      const int nops = vcode_count_ops();
      for (int j = 0; j < nops; j++) {
         if (vcode_get_jit_addr(j) > rip) {
            if (j == 0) {
               vcode_select_block(i - 1);
               mark_op = vcode_count_ops() - 1;
            }
            else
               mark_op = j - 1;
            goto found_op;
         }
         else
            mark_op = j;
      }
   }
 found_op:
   jit_dump(jc, mark_op);

   if (signum == SIGTRAP)
      color_printf("$red$Hit JIT breakpoint$$\n\n");
   else
      color_printf("$red$Crashed while running JIT compiled code$$\n\n");

   printf("RAX %16llx    RSP %16llx    RIP %16llx\n",
          uc->uc_mcontext.gregs[REG_RAX], uc->uc_mcontext.gregs[REG_RSP],
          uc->uc_mcontext.gregs[REG_RIP]);
   printf("RBX %16llx    RBP %16llx    EFL %16llx\n",
          uc->uc_mcontext.gregs[REG_RBX], uc->uc_mcontext.gregs[REG_RBP],
          uc->uc_mcontext.gregs[REG_EFL]);
   printf("RCX %16llx    RSI %16llx\n",
          uc->uc_mcontext.gregs[REG_RCX], uc->uc_mcontext.gregs[REG_RSI]);
   printf("RDX %16llx    RDI %16llx\n",
          uc->uc_mcontext.gregs[REG_RDX], uc->uc_mcontext.gregs[REG_RDI]);

   printf("\n");

   const uint32_t *const stack_top =
      (uint32_t *)((uc->uc_mcontext.gregs[REG_RBP] + 3) & ~7);

   for (int i = (jc->stack_size + 15) / 16; i > 0; i--) {
      const uint32_t *p = stack_top - i * 4;
      printf("%p  RBP-0x%-2x  %08x %08x %08x %08x\n", p, i * 16,
             p[0], p[1], p[2], p[3]);
   }

   for (int i = 0; i <= (jc->params_size + 15) / 16; i++) {
      const uint32_t *p = stack_top + i * 4;
      printf("%p  RBP+0x%-2x  %08x %08x %08x %08x\n", p, i * 16,
             p[0], p[1], p[2], p[3]);
   }
#endif
}