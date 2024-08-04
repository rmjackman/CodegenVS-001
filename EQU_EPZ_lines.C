int symtab_epz = 0;			// First address entry (after constants)
int symtab_init = 0;		// can be at beginning of constants (EQU)
							//        or beginning of addresses (EPZ)

void disassemble(const char* filename) {
	
	int i = 0;


/********      Find first address entry  *******/

	symtab_epz = 0;
	while (sss[symtab_epz].symtyp=='[')
		symtab_epz++;
	

/** for all commands except NN commands ********/

	symtab_init = symtab_epz;

/****************************/


	case NN:
		symtab_init = 0;


/****************************/

	i = symtab_init;
