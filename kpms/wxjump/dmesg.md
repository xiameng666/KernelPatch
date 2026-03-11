rubens:/ $ dmesg | grep wx
[ 4957.724899] [T423361] [+] KP D     name: wxjump
[ 4957.724941] [T423361] wxjump: ====== INIT START ======
[ 4957.724943] [T423361] wxjump: args=(null)
[ 4957.724944] [T423361] wxjump: ====== resolve_symbols START ======
[ 4957.725768] [T423361] wxjump:   find_vma         = ffffffe2a82b373c
[ 4957.725947] [T423361] wxjump:   get_task_mm      = ffffffe2a7f268c0
[ 4957.726116] [T423361] wxjump:   mmput            = ffffffe2a7f2684c
[ 4957.726897] [T423361] wxjump: exit_mmap at ffffffe2a82b4ed8
[ 4957.728505] [T423361] wxjump:   __get_free_pages = ffffffe2a82d6254
[ 4957.728506] [T423361] wxjump:   free_pages       = ffffffe2a82d5588
[ 4957.752832] [T623361] wxjump:   memstart_addr    = ffffffe2aa2d5b20 (val=0x40000000)
[ 4957.752840] [T623361] wxjump:   physvirt_offset  = 0000000000000000
[ 4957.752843] [T623361] wxjump:   TCR_EL1=0x1b2b5593519 T1SZ=25 PAGE_OFFSET=0xffffff8000000000
[ 4957.752846] [T623361] wxjump:   test_page alloc  = 0xffffff802ca74000
[ 4957.752848] [T623361] wxjump:   PAR_EL1=0xff0000006ca74b80 (fault=0)
[ 4957.752849] [T623361] wxjump:   physvirt_offset  = 0xffffff7fc0000000 (detected)
[ 4957.752852] [T623361] wxjump: page_shift=12 page_level=3
[ 4957.767433] [T623361] wxjump:   _raw_spin_lock   = ffffffe2a9a0df70
[ 4957.767442] [T623361] wxjump:   _raw_spin_unlock = ffffffe2a9a0db84
[ 4957.780379] [T623361] wxjump:   flush_tlb_page   = 0000000000000000
[ 4957.781161] [T623361] wxjump:   __flush_tlb_range= ffffffe2a829a5cc
[ 4957.829709] [T623361] wxjump:   down_read_trylock= ffffffe2a7fea320
[ 4957.829714] [T623361] wxjump:   up_read          = ffffffe2a7fe7dcc
[ 4957.836721] [T623361] wxjump: page fault handler at ffffffe2a9a10778
[ 4957.836724] [T623361] wxjump:   kzalloc          = ffffffe2a8300f64
[ 4957.836725] [T623361] wxjump:   kcalloc          = 0000000000000000
[ 4957.836727] [T623361] wxjump:   kfree            = ffffffe2a82fb2d4
[ 4957.836728] [T623361] wxjump:   icache_range     = ffffffe2a7e1590c
[ 4957.836729] [T623361] wxjump:   copy_nofault     = ffffffe2a8235258
[ 4957.836730] [T623361] wxjump: ====== resolve_symbols OK ======
[ 4957.836732] [T623361] wxjump: KP offsets: pgd=72 mm=-1 active_mm=1312
[ 4957.836733] [T623361] wxjump: scanning vma offsets...
[ 4957.836736] [T623361] wxjump: vm_mm offset: 0x40
[ 4957.836737] [T623361] wxjump: vma_vm_mm_offset=0x40
[ 4957.846973] [T623361] wxjump: mm_offset=0x518 (active_mm-8)
[ 4957.846976] [T623361] wxjump: context.id at offset 0x300 (asid@bits[0:15])
[ 4957.846978] [T623361] wxjump: context_id_offset=0x300 asid_shift=0
[ 4957.846980] [T623361] wxjump: mmap_lock at offset 112 (verified)
[ 4957.847009] [T623361] wxjump: hooked page fault handler
[ 4957.847018] [T623361] wxjump: hooked exit_mmap
[ 4957.847025] [T623361] wxjump: hooked prctl
[ 4957.847026] [T623361] wxjump: ====== INIT OK ======
[ 4957.847028] [T623361] [+] KP I load_module: [wxjump] succeed with [(null)]
[ 4957.847151] [T202613] wxjump: fault #1 FAR=12d4b000 ESR=92000047 task=ffffff801f7e2500
[ 4957.847234] [T723301] wxjump: fault #2 FAR=70ddccd0 ESR=82000007 task=ffffff8004b98000
[ 4957.847268] [T723301] wxjump: fault #3 FAR=12e88000 ESR=92000047 task=ffffff8004b98000
[ 4957.847542] [T723301] wxjump: fault #4 FAR=12e891e0 ESR=92000047 task=ffffff8004b98000
[ 4957.847827] [T723301] wxjump: fault #5 FAR=12e8a000 ESR=92000047 task=ffffff8004b98000
[ 4957.855880] [T723361] [+] KP D get_module_info: name=wxjump
[ 4976.826704] [T623739] wxjump: [patch] page=72d1f2f000 offset=256 len=20
[ 4976.826719] [T623739] wxjump: patched new shadow at 72d1f2f000+256 (20 bytes) orig_pfn=1d9e3a shadow_pfn=810c9
rubens:/ $