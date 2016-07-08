/*----------------------------------------------------------------------------
 * Copyright (c) 2013-2015, The Regents of the University of California (Regents).
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Regents nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
 * SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING
 * OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF REGENTS HAS
 * BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE. THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY, PROVIDED
 * HEREUNDER IS PROVIDED "AS IS". REGENTS HAS NO OBLIGATION TO PROVIDE
 * MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 *----------------------------------------------------------------------------
 * Copyright (c) 2015-2016, University of Cambridge.
 * All Rights Reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University of Cambridge nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * IN NO EVENT SHALL UNIVERSITY OF CAMBRIDGE BE LIABLE TO ANY PARTY FOR DIRECT,
 * INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS,
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF REGENTS
 * HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * UNIVERSITY OF CAMBRIDGE SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE. THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY,
 * PROVIDED HEREUNDER IS PROVIDED "AS IS". UNIVERSITY OF CAMBRIDGE HAS NO
 * OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
 * MODIFICATIONS.
 *----------------------------------------------------------------------------*/

#include <elf.h>
#include <string.h>
#include <stdio.h>

int load_elf(const uint8_t *elf, const uint32_t elf_size) {
  // sanity checks
  if(elf_size <= sizeof(Elf64_Ehdr))
    return 1;                   /* too small */

  const Elf64_Ehdr *eh = (const Elf64_Ehdr *)elf;
  if(!IS_ELF64(*eh))
    return 2;                   /* not a elf64 file */

  const Elf64_Phdr *ph = (const Elf64_Phdr *)(elf + eh->e_phoff);
  if(elf_size < eh->e_phoff + eh->e_phnum*sizeof(*ph))
    return 3;                   /* internal damaged */

  uint32_t i;
  for(i=0; i<eh->e_phnum; i++) {
    if(ph[i].p_type == PT_LOAD && ph[i].p_memsz) { /* need to load this physical section */
      if(ph[i].p_filesz) {                         /* has data */
        if(elf_size < ph[i].p_offset + ph[i].p_filesz)
          return 3;             /* internal damaged */
        memcpy((uint8_t *)ph[i].p_paddr, elf + ph[i].p_offset, ph[i].p_filesz);
      }
      if(ph[i].p_memsz > ph[i].p_filesz) { /* zero padding */
        memset((uint8_t *)ph[i].p_paddr + ph[i].p_filesz, 0, ph[i].p_memsz - ph[i].p_filesz);
      }
    }
  }

  return 0;
}
