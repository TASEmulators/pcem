void voodoo_queue_command(voodoo_t *voodoo, uint32_t addr_type, uint32_t val);
void voodoo_flush(voodoo_t *voodoo);
void voodoo_execute_commands(voodoo_set_t *set, voodoo_t *voodoo);
void voodoo_wait_for_swap_complete(voodoo_t *voodoo);
void voodoo_execute_command(voodoo_t *voodoo);
