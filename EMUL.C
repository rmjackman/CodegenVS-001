#define aaaa *( xlbuf  +  *(pcptr+1)  +  ( *(pcptr+2)<<8 )  )
#define aa *(xlbuf + *(pcptr+1))


emul(unsigned int pc)
	{
	char acc,xxx,yyy,sp;
	char xlbuf[0xffffl];
	
	pcptr = xlbuf + pc

	switch(*pcptr)
		{	
		case 0:	err();
		case 1:	acc |= ???
		case 2:
		case 3:
		case 4:	err();
		
/* ORA aa */
		case 5:	acc |= aa;
					nfg = 128 & acc;
					zfg = acc;
					pcptr += 2;
					break;
					
/* ASL aa */
		case 6:	cfg = 128 & aa;
					*(xlbuf + *pcptr) <<= 1;
					pcptr += 2;
					break;
					
		case 7:	err();
		
/* PHP */
		case 8:	*(--sp) = ???
		
/* ORA #nn */
		case 9:	acc |= (*(pcptr+1));
					nfg = 128 & acc;
					zfg = acc;
					pcptr += 2;
					break;
					
/* ASL */
		case 10:	cfg = 128 & aa;
					acc <<= 1;
					pcptr++;
					break;
					
		case 11:	
		case 12:	err();
		
/* ORA aaaa */
		case 13:	acc |= aaaa;
					nfg = 128 & acc;
					zfg = acc;
					pcptr += 2;
					break;
					
/* ASL aaaa */
		case 14:	cfg = 128 & aaaa;
					aaaa <<= 1;
					nfg = 128 & acc;
					zfg = acc;
					pcptr += 2;
					break;

		case 15:	err();
		
/* BPL dd */
		case 16:	if (!nfg)	pcptr += 2 + (signed char) *(pcptr + 1); 
					else pcptr += 2;
					
/* ORA (aa),Y */
		case 17:				
		}
	}