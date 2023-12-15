

internal u32
str_length(char *str)
{
	u32 Result = 0;
	for (char *c = str; *c != '\0'; c++) {
		Result++;
	}
	return(Result);
}

internal string_u8
str_u8(char *str)
{
	string_u8 Result = {};
	Result.data = (u8 *)str;
	for (char *c = str; *c != '\0'; c++) {
		Result.length++;
	}
	return(Result);
}

internal b32
str_u8_are_same(string_u8 *str1, string_u8 *str2)
{
	b32 Result = true;

	if (str1->length == str2->length) {
		 u8 *c1 = str1->data;
		 u8 *c2 = str2->data;
		for (u32 index = 0; index < str1->length; index++) {
			if (*c1++ != *c2++) {
				Result = false;
				break;
			}
		}
	} else {
		Result = false;
	}
	return(Result);
}
