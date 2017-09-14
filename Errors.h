#define MM_OK ((HRESULT)0x00000000L)
#define MM_ERROR ((HRESULT)0x00000001L)
#define MM_DISP_CHANGE_UNSUCCESSFUL ((HRESULT)0x00000002L)
#define MM_INVALID_DC ((HRESULT)0x00000003L)
#define MM_INVALID_PIXEL_FORMAT ((HRESULT)0x00000004L)
#define MM_HRC ((HRESULT)0x00000005L)
#define MM_OUT_OF_MEMORY ((HRESULT)0x00000006L)
#define MM_INVALID_PARAMS ((HRESULT)0x00000007L)
#define MM_FILE_NOT_FOUND ((HRESULT)0x00000008L)
#define MM_HIDDEN ((HRESULT)0x00000009L)		//po tomto ziaden dialog window
#define MM_UNSUCCESFUL ((HRESULT)0x0000000AL)

//GL
#define GLL_NO_ERROR		((HRESULT)0x00000000L)
#define GLL_INVALID_ENUM	((HRESULT)0x00000500L)
#define GLL_INVALID_VALUE	((HRESULT)0x00000501L)
#define GLL_INVALID_OPERATION	((HRESULT)0x00000502L)
#define GLL_STACK_OVERFLOW	((HRESULT)0x00000503L)
#define GLL_STACK_UNDERFLOW	((HRESULT)0x00000504L)
#define GLL_OUT_OF_MEMORY	((HRESULT)0x00000505L)

LPCTSTR strerror(HRESULT hr){
	switch (hr){
		case MM_ERROR: return "Unknown error occurred";
		case MM_DISP_CHANGE_UNSUCCESSFUL: return "Disp change unsuccessful";
		case MM_INVALID_DC: return "Invalid DC";
		case MM_INVALID_PIXEL_FORMAT: return "Invalid pixel format";
		case MM_HRC: return "Unable to create rendering context";
		case MM_OUT_OF_MEMORY: return "Out of system or video memory";
		case MM_INVALID_PARAMS: return "Invalid parameters";
		case MM_FILE_NOT_FOUND: return "File not found";
		case MM_UNSUCCESFUL: return "Call wasn't succesful";
		//GL
		case GLL_INVALID_ENUM: return "Enum argument out of range";
		case GLL_INVALID_VALUE: return "Numeric argument out of range";
		case GLL_INVALID_OPERATION: return "Operation illegal in current state";
		case GLL_STACK_OVERFLOW: return "Command would cause a stack overflow";
		case GLL_STACK_UNDERFLOW: return "Command would cause a stack underflow";
		case GLL_OUT_OF_MEMORY:return "Not enough memory left to execute command";
	}
	return "File not found or another error";
}