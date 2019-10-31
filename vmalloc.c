/*
 * Walk a vmap address to the struct page it maps.
 */
struct page *vmalloc_to_page(const void *vmalloc_addr)
{
	unsigned long addr = (unsigned long) vmalloc_addr;
	struct page *page = NULL;
	pgd_t *pgd = pgd_offset_k(addr);
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *ptep, pte;

	/*
	 * XXX we might need to change this if we add VIRTUAL_BUG_ON for
	 * architectures that do not vmalloc module space
	 */
	VIRTUAL_BUG_ON(!is_vmalloc_or_module_addr(vmalloc_addr));

	if (pgd_none(*pgd))
		return NULL;
	p4d = p4d_offset(pgd, addr);
	if (p4d_none(*p4d))
		return NULL;
	pud = pud_offset(p4d, addr);

	/*
	 * Don't dereference bad PUD or PMD (below) entries. This will also
	 * identify huge mappings, which we may encounter on architectures
	 * that define CONFIG_HAVE_ARCH_HUGE_VMAP=y. Such regions will be
	 * identified as vmalloc addresses by is_vmalloc_addr(), but are
	 * not [unambiguously] associated with a struct page, so there is
	 * no correct value to return for them.
	 */
	WARN_ON_ONCE(pud_bad(*pud));
	if (pud_none(*pud) || pud_bad(*pud))
		return NULL;
	pmd = pmd_offset(pud, addr);
	WARN_ON_ONCE(pmd_bad(*pmd));
	if (pmd_none(*pmd) || pmd_bad(*pmd))
		return NULL;

	ptep = pte_offset_map(pmd, addr);
	pte = *ptep;
	if (pte_present(pte))
		page = pte_page(pte);
	pte_unmap(ptep);
	return page;
}
