/****************************************************************************/

//#define DEBUG

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <winnls.h>
//#include <ctype.h>

#include "dds.h"

#define BUF_SIZE	1024
#define BUF_SIZE2	128
#define BUF_SIZE_ICON_PATH	( 64 * 1024 )

#define MAX_FOLDER_NUM	50
#define MAX_SPOT_NUM	50

/*** macro ******************************************************************/

#define StrCpy_fromP2P( dst, from, to ) \
	while(( from ) < ( to )) ( *( dst )++ = *( from )++ )

#define SearchExt( p )	StrTokFile( NULL, ( p ), STF_EXT )
#define ZeroToNull( p )	( *( p ) ? ( p ) : NULL )
#define bzero( p, s ) memset(( p ), 0, ( s ))

/*** enum *******************************************************************/

enum{
	STF_DRV		= 1 << 0,
	STF_PATH	= 1 << 1,
	STF_NODE	= 1 << 2,
	STF_EXT		= 1 << 3,
	STF_SHORT	= 1 << 4,
	STF_LONG	= 1 << 5,
	STF_FULL	= 1 << 6,
	
	STF_NAME	= STF_NODE | STF_EXT,
	STF_PATH2	= STF_DRV  | STF_PATH
};

enum {
	#define KML_KEYWORD( id, word ) id,
	#include "kml_keyword.h"
	#undef KML_KEYWORD
	KML_LAST
};

enum {
	MODE_NONE,
	MODE_FOLDER,
	MODE_SPOT,
	MODE_STYLE_ID,
};

enum {
	MODE_TXT,
	MODE_CSV,
	MODE_KML
};

/*** new type ***************************************************************/

typedef struct {
	UINT	uIcon;
	WCHAR	szName[ 102 / 2 ];
	WCHAR	szComment[ 194 / 2 ];
	UINT	uLati;
	UINT	uLong;
} MAPLUS_SPOT;

typedef struct {
	WCHAR		szName[ 22 ];
	UINT		uNum;
	MAPLUS_SPOT	Spot[ MAX_SPOT_NUM ];
} MAPLUS_FOLDER;

typedef struct {
	UCHAR			cHeader[ 0x24 ];
	MAPLUS_FOLDER	Folder[ MAX_FOLDER_NUM ];
} MAPLUS_DATA;

typedef struct {
	UINT	uID;
	UCHAR	*szPath;
} ICON_TBL;

/*** gloval var *************************************************************/

MAPLUS_DATA		*g_pData;

UCHAR	g_szBuf			[ BUF_SIZE ];
UCHAR	g_szBuf2		[ BUF_SIZE ];
UCHAR	g_szFolder		[ BUF_SIZE ];
UCHAR	g_szSpot		[ BUF_SIZE ];
UCHAR	g_szComment		[ BUF_SIZE ];
UCHAR	g_szPrevFolder	[ BUF_SIZE ];
UCHAR	g_szLati		[ BUF_SIZE2 ];
UCHAR	g_szLong		[ BUF_SIZE2 ];

UCHAR	g_szDegFormat	[ BUF_SIZE2 ];

UCHAR	g_szFolderHeader[ BUF_SIZE2 ];
UCHAR	g_szFolderFooter[ BUF_SIZE2 ];
UCHAR	g_szSpotBody	[ BUF_SIZE ];

FILE	*g_fpCfg;

ICON_TBL	g_IconTbl[ 500 ];				// アイコンテーブル
UCHAR		*g_pIconPathBuf;
UCHAR		*g_pIconPathBufTail;
UINT		g_uIconNum = 0;

/*
	0020 0021 0022 0023 0024 0025 0026 0027
	0028 0029 002a 002b 002c 002d 002e 002f
	0030 0031 0032 0033 0034 0035 0036 0037
	0038 0039 003a 003b 003c 003d 003e 003f
	0040 0041 0042 0043 0044 0045 0046 0047
	0048 0049 004a 004b 004c 004d 004e 004f
	0050 0051 0052 0053 0054 0055 0056 0057
	0058 0059 005a 005b 005c 005d 005e 005f
	0060 0061 0062 0063 0064 0065 0066 0067
	0068 0069 006a 006b 006c 006d 006e 006f
	0070 0071 0072 0073 0074 0075 0076 0077
	0078 0079 007a 007b 007c 007d 007e 0020
*/
const WCHAR g_cZenkaku[] = { // アルファベット
	0x3000, 0xff01, 0x201d, 0xff03, 0xff04, 0xff05, 0xff06, 0x2019,
	0xff08, 0xff09, 0xff0a, 0xff0b, 0xff0c, 0xff0d, 0xff0e, 0xff0f,
	0xff10, 0xff11, 0xff12, 0xff13, 0xff14, 0xff15, 0xff16, 0xff17,
	0xff18, 0xff19, 0xff1a, 0xff1b, 0xff1c, 0xff1d, 0xff1e, 0xff1f,
	0xff20, 0xff21, 0xff22, 0xff23, 0xff24, 0xff25, 0xff26, 0xff27,
	0xff28, 0xff29, 0xff2a, 0xff2b, 0xff2c, 0xff2d, 0xff2e, 0xff2f,
	0xff30, 0xff31, 0xff32, 0xff33, 0xff34, 0xff35, 0xff36, 0xff37,
	0xff38, 0xff39, 0xff3a, 0xff3b, 0xffe5, 0xff3d, 0xff3e, 0xff3f,
	0x2018, 0xff41, 0xff42, 0xff43, 0xff44, 0xff45, 0xff46, 0xff47,
	0xff48, 0xff49, 0xff4a, 0xff4b, 0xff4c, 0xff4d, 0xff4e, 0xff4f,
	0xff50, 0xff51, 0xff52, 0xff53, 0xff54, 0xff55, 0xff56, 0xff57,
	0xff58, 0xff59, 0xff5a, 0xff5b, 0xff5c, 0xff5d, 0xff5e, 0x3000,
};

/*
	     ff61 ff62 ff63 ff64 ff65 ff66 ff67
	ff68 ff69 ff6a ff6b ff6c ff6d ff6e ff6f
	ff70 ff71 ff72 ff73 ff74 ff75 ff76 ff77
	ff78 ff79 ff7a ff7b ff7c ff7d ff7e ff7f
	ff80 ff81 ff82 ff83 ff84 ff85 ff86 ff87
	ff88 ff89 ff8a ff8b ff8c ff8d ff8e ff8f
	ff90 ff91 ff92 ff93 ff94 ff95 ff96 ff97
	ff98 ff99 ff9a ff9b ff9c ff9d ff9e ff9f
*/

const WCHAR g_cZenkaku2[] = {	// 0xA1～0xDF
	        0x3002, 0x300c, 0x300d, 0x3001, 0x30fb, 0x30f2, 0x30a1,
	0x30a3, 0x30a5, 0x30a7, 0x30a9, 0x30e3, 0x30e5, 0x30e7, 0x30c3,
	0x30fc, 0x30a2, 0x30a4, 0x30a6, 0x30a8, 0x30aa, 0x30ab, 0x30ad,
	0x30af, 0x30b1, 0x30b3, 0x30b5, 0x30b7, 0x30b9, 0x30bb, 0x30bd,
	0x30bf, 0x30c1, 0x30c4, 0x30c6, 0x30c8, 0x30ca, 0x30cb, 0x30cc,
	0x30cd, 0x30ce, 0x30cf, 0x30d2, 0x30d5, 0x30d8, 0x30db, 0x30de,
	0x30df, 0x30e0, 0x30e1, 0x30e2, 0x30e4, 0x30e6, 0x30e8, 0x30e9,
	0x30ea, 0x30eb, 0x30ec, 0x30ed, 0x30ef, 0x30f3, 0x309b, 0x309c,
};

const UCHAR *g_szKmlKeyword[] = {
	#define KML_KEYWORD( id, word ) word,
	#include "kml_keyword.h"
	#undef KML_KEYWORD
};

/****************************************************************************/

#ifdef DEBUG
char *PrintDeg( double d ){
	
	static char szBuf[ 256 ];
	
	d *= 3600;
	
	sprintf(
		szBuf,
		"%3d:%02d:%5.2f",
		( UINT )d / 3600,
		( UINT )d % 3600 / 60,
		d - ( UINT )( d / 60 ) * 60
	);
	
	return szBuf;
}
#endif

/*** 測地系変換 (GpsLogCv のコピペw) ****************************************/

#define PI 3.1415926535897932384626433832795028841971693993751
#define TORAD	(PI/180.0)

typedef struct {
	union {
		double lat;
		double x;
	};
	
	union {
		double lon;
		double y;
	};
	
	union {
		double alt;
		double z;
	};
} cordinate_t;

typedef struct {
	char *datum_name;
	double da;
	double df;
	double dx;
	double dy;
	double dz;
} Molodensky_param_t;

static Molodensky_param_t mTokyo = {
	"WGS84toTokyo",
	-739.845,
	-0.10037483e-4,
	128.0,
	-481.0,
	-664.0
};

static Molodensky_param_t mWGS = {
	"TokyoToWGS84",
	739.845,
	0.10037483e-4,
	-128.0,
	481.0,
	664.0
};

#if 1
void Molodensky( cordinate_t *Pos, Molodensky_param_t *datum ){
	double a, c, ee, f, rn, rm;
	double coslat, coslon, sinlat, sinlon, esnlt2;
	
	Pos->lat *= TORAD;
	Pos->lon *= TORAD;
	
	sinlat = sin(Pos->lat);
	coslat = cos(Pos->lat);
	sinlon = sin(Pos->lon);
	coslon = cos(Pos->lon);
	
	a = 6378137.0 - datum->da;
	f =	0.00335281066474 - datum->df;
	c = (1.0 - f);
	ee = f*(2.0 - f);
	
	esnlt2 = sqrt(1.0 - ee * sinlat*sinlat);
	
	rn = a / esnlt2;
	rm = (a * (1.0 - ee)) / (esnlt2*esnlt2*esnlt2);
	
	Pos->lat +=
		( datum->dz * coslat 
		+ (  datum->da * ee * coslat / esnlt2
		   - datum->dx * coslon
		   - datum->dy * sinlon
		   + datum->df * ((rm /c) + (rn * c)) * coslat
		   ) * sinlat
		)
		/ (rm + Pos->alt);
	
	Pos->lon += ( (datum->dy * coslon) - (datum->dx * sinlon) ) / ((rn + Pos->alt) * coslat);
	
	Pos->alt += 
		 (datum->dx * coslon
		+ datum->dy * sinlon) * coslat
		+(datum->dz 
		+ datum->df * c * rn * sinlat) * sinlat
		- (datum->da * esnlt2);
	
	Pos->lat /= TORAD;
	Pos->lon /= TORAD;
}

#else

//
// 測地系変換
// Nowral
// 99/11/23
//
void llh2xyz( cordinate_t *Pos, double a, double e2 ) { // 楕円体座標 -> 直交座標
	double lat = Pos->lat;
	double lon = Pos->lon;
	double h   = Pos->alt;
	double sb = sin(lat);
	double cb = cos(lat);
	double rn = a / sqrt(1-e2*sb*sb);
	
	Pos->x = (rn+h) * cb * cos(lon);
	Pos->y = (rn+h) * cb * sin(lon);
	Pos->z = (rn*(1-e2)+h) * sb;
}

void xyz2llh( cordinate_t *Pos, double a, double e2 ) { // 直交座標 -> 楕円体座標
	double bda = sqrt(1-e2);
	
	double p = sqrt(Pos->x*Pos->x + Pos->y*Pos->y);
	double t = atan2(Pos->z, p*bda);
	double st = sin(t);
	double ct = cos(t);
	double lat = atan2(Pos->z+e2*a/bda*st*st*st, p-e2*a*ct*ct*ct);
	double lon = atan2(Pos->y, Pos->x);
	
	double sb = sin(lat);
	double rn = a / sqrt(1-e2*sb*sb);
	double h = p/cos(lat) - rn;
	
	Pos->lat = lat;
	Pos->lon = lon;
	Pos->alt = h;
}

void Molodensky( cordinate_t *Pos, Molodensky_param_t *datum ){
	// データム諸元
	// (WGS 84)
	double a_w = 6378137;          // 赤道半径
	double f_w = 1 / 298.257223;   // 扁平率
	double e2_w = 2*f_w - f_w*f_w; // 第1離心率
	// (Tokyo)
	double a_t = 6377397.155;
	double f_t = 1 / 299.152813;
	double e2_t = 2*f_t - f_t*f_t;
	// 並行移動量 [m]
	// e.g. x_t + dx_t = x_w etc.
	double dx_t = -148;
	double dy_t =  507;
	double dz_t =  681;
	
	#ifdef DEBUG
		printf( "%.20lf %.20lf\n", Pos->lat, Pos->lon );
		printf( "%s/", PrintDeg( Pos->lat ));
		printf( "%s--> ", PrintDeg( Pos->lon ));
	#endif
	
	Pos->lat *= TORAD;
	Pos->lon *= TORAD;
	// 測地系変換
	if( datum == &mTokyo ) { // WGS 84 -> Tokyo
		llh2xyz( Pos, a_w, e2_w );
		
		Pos->x -= dx_t;
		Pos->y -= dy_t;
		Pos->z -= dz_t;
		
		xyz2llh( Pos, a_t, e2_t);
		
	}else{ // Tokyo -> WGS 84
		llh2xyz( Pos, a_t, e2_t);
		
		Pos->x += dx_t;
		Pos->y += dy_t;
		Pos->z += dz_t;
		
		xyz2llh( Pos, a_w, e2_w);
	}
	Pos->lat /= TORAD;
	Pos->lon /= TORAD;
	
	#ifdef DEBUG
		printf( "%s/", PrintDeg( Pos->lat ));
		printf( "%s\n", PrintDeg( Pos->lon ));
	#endif
}
#endif

/*** strtokfile *************************************************************/

UCHAR *StrTokFile( UCHAR *szDst, UCHAR *szFileName, UCHAR cMode ){
	
	UCHAR	*pPath,
			*pNode,
			*pExt,
			*pSce = szFileName,
			*pDst = szDst;
	
	DWORD	dwLen;
	
	if( szDst ){
		// full path 変換
		if( cMode & STF_FULL ){
			if(( dwLen = GetFullPathName( pSce, MAX_PATH, szDst, NULL )) && dwLen < MAX_PATH )
				pSce = szDst;
		}
		
		// LFN 変換
		if( cMode & STF_LONG ){
			if(( dwLen = GetLongPathName( pSce, szDst, MAX_PATH )) && dwLen < MAX_PATH )
				pSce = szDst;
			
		// SFN 変換
		}else if( cMode & STF_SHORT ){
			if(( dwLen = GetShortPathName( pSce, szDst, MAX_PATH )) && dwLen < MAX_PATH )
				pSce = szDst;
		}
	}
	
	// パス名
	if( pSce[ 0 ] != '\0' && pSce[ 1 ] == ':' )
		pPath = pSce + 2;
	else
		pPath = pSce;
	
	// ノード名
	if( pNode = strrchr( pPath, '\\' )) ++pNode;
	else pNode = pPath;
	
	// 拡張子
	if( !( pExt = strrchr( pNode, '.' ))) pExt = strchr( pNode, '\0' );
	
	if( szDst == NULL ){
		if( cMode & STF_PATH )	return( ZeroToNull( pPath ));
		if( cMode & STF_NODE )	return( ZeroToNull( pNode ));
		if( cMode & STF_EXT )	return( ZeroToNull( pExt ));
		return( pSce );
	}
	
	// szDst にコピー
	if(( cMode & ( STF_DRV | STF_PATH | STF_NODE | STF_EXT )) == 0 ){
		strcpy( pDst, pSce );
	}else{
		if( cMode & STF_DRV )  StrCpy_fromP2P( pDst, pSce,  pPath );
		if( cMode & STF_PATH ) StrCpy_fromP2P( pDst, pPath, pNode );
		if( cMode & STF_NODE ) StrCpy_fromP2P( pDst, pNode, pExt );
		
		*pDst = '\0';
		
		if( cMode & STF_EXT )  strcpy( pDst, pExt );
	}
	
	return( ZeroToNull( szDst ));
}

/*** IsExt ******************************************************************/

BOOL IsExt( UCHAR *szFileName, UCHAR *szExt ){
	
	UCHAR *pExt;
	
	return(
		( pExt = SearchExt( szFileName )) == NULL ?
			szExt == NULL :
			!stricmp( ++pExt, szExt )
	);
}

/*** change ext *************************************************************/

UCHAR *ChangeExt( UCHAR *szDst, UCHAR *szFileName, UCHAR *szExt ){
	
	StrTokFile( szDst, szFileName, STF_PATH2 | STF_NODE );
	return(
		( szExt )	? strcat( strcat( szDst, "." ), szExt )
					: szDst
	);
}

/*** UNICODE 半角→全角変換 *************************************************/

WCHAR *ConvHan2Zen( WCHAR *szDst, WCHAR *szSrc ){
	
	WCHAR *p = szDst;
	
	while( *szSrc ){
		if( 0x0020 <= *szSrc && *szSrc <= 0x007F ){			// 半角
			*p++ = g_cZenkaku[ *szSrc - 0x0020 ];
		}else if( 0xFF61 <= *szSrc && *szSrc <= 0xFF9F ){	// 半角
			*p++ = g_cZenkaku2[ *szSrc - 0xFF61 ];
		}else{												// 全角?
			*p++ = *szSrc;
		}
		
		++szSrc;
	}
	
	*p = '\0';
	
	return( szDst );
}

/*** tab ⇔ csv 変換 ********************************************************/

UCHAR *ConvTab2Csv( UCHAR *szStr ){
	UCHAR *s = szStr;
	for(; *s; ++s ) if( *s == '\t' ) *s = ',';
	return( szStr );
}

UCHAR *ConvCsv2Tab( UCHAR *szStr ){
	UCHAR	*s = szStr;
	UCHAR	*d = szStr;
	BOOL	bQuote = FALSE;
	
	DebugMsg( ">>%s", szStr );
	
	do {
		if( *s == ',' && !bQuote ){
			// "～" 内でなければ \t に変換
			*d++ = '\t';
		}else if( *s == '"' ){
			if( s[ 1 ] == '"' ){
				// "" → " 変換
				*d++ = '"';
				++s;
			}else if(
				bQuote && ( s[ 1 ] == ',' || s[ 1 ] == '\0' ) ||
				!bQuote && ( s == szStr || s[ -1 ] == ',' )
			){
				bQuote = !bQuote;
			}else{
				*d++ = *s;
			}
		}else{
			*d++ = *s;
		}
	}while( *s++ );
	
	DebugMsg( "<<%s", szStr );
	return( szStr );
}

/*** config file ************************************************************/

UCHAR *SearchCfgKey( UCHAR *szBuf, UINT uSize, UCHAR *szKeywd ){
	
	UINT	uLen = strlen( szKeywd );
	
	fseek( g_fpCfg, 0, SEEK_SET );
	
	while( fgets( szBuf, uSize, g_fpCfg )){
		if( strncmp( szBuf, szKeywd, uLen ) == 0 ){
			return( szBuf );
		}
	}
	
	printf( "can't find key %s\n", szKeywd );
	
	return( NULL );
}

UCHAR *GetCfgData( UCHAR *szBuf, UINT uSize, UCHAR *szKeywd ){
	
	UCHAR *p;
	
	if( SearchCfgKey( szBuf, uSize, szKeywd ) == NULL ) return( NULL );
	
	while( fgets( szBuf, uSize, g_fpCfg )){
		// 他のキー
		if( *szBuf == '[' ) return( NULL );
		
		// コメントでも空行でもない
		if( *szBuf != ';' && *szBuf != '\x0A' ){
			if( p = strchr( szBuf, '\x0A' )) *p = '\0';	// \n 削除
			return( szBuf );
		}
	}
	
	return( NULL );
}

UCHAR *GetCfgAllData( UCHAR *szBuf, UINT uSize, UCHAR *szKeywd ){
	
	UCHAR *p = szBuf;
	
	if( SearchCfgKey( szBuf, uSize, szKeywd ) == NULL ) return( NULL );
	
	*p = '\0';
	
	while( fgets( p, uSize, g_fpCfg )){
		// 他のキー
		if( *p == '[' ){
			*p = '\0';
			break;
		}
		
		p = strchr( p, '\0' );
	}
	
	return( szBuf );
}

/*** icon マッピング ********************************************************/

#define ICON_NONE	0xFFFFFFFF

UINT GetIconID( UCHAR *szIconPath, BOOL bErrorMode ){
	int i;
	for( i = 0; i < ( int )g_uIconNum; ++i ){
		if( !strcmp( g_IconTbl[ i ].szPath, szIconPath )) return( g_IconTbl[ i ].uID );
	}
	return( bErrorMode ? ICON_NONE : g_IconTbl[ 0 ].uID );
}

UCHAR *GetIconPath( UINT uIconID ){
	int i;
	for( i = 0; i < ( int )g_uIconNum; ++i ){
		 if( g_IconTbl[ i ].uID == uIconID ) return( g_IconTbl[ i ].szPath );
	}
	
	return( g_IconTbl[ 0 ].szPath );
}

void AddIcon( UINT uID, UCHAR *szIconPath ){
	UINT	u;
	
	if(( u = GetIconID( szIconPath, 1 )) == ICON_NONE ){
		g_IconTbl[ g_uIconNum ].uID		= uID;
		g_IconTbl[ g_uIconNum ].szPath	= g_pIconPathBufTail;
		
		++g_uIconNum;
		g_pIconPathBufTail = strchr( g_pIconPathBufTail, '\0' ) + 1;
	}else{
		g_IconTbl[ u ].uID		= uID;
	}
}

UCHAR *GetCfgIconTable( void ){
	
	UINT	uIconID;
	
	if( SearchCfgKey( g_szBuf, sizeof( g_szBuf ), "[icon]" ) == NULL ) return( NULL );
	
	while( fgets( g_szBuf, sizeof( g_szBuf ), g_fpCfg ) && *g_szBuf != '[' ){
		if( *g_pIconPathBufTail != ';' && sscanf( g_szBuf, "%u%*[ \t]%[^\n]", &uIconID, g_pIconPathBufTail ) == 2 ){
			AddIcon( uIconID, g_pIconPathBufTail );
		}
	}
	return( g_szBuf );
}

/*** kml→maplus ************************************************************/

enum {
	XML_PARSE_1ST,
	XML_PARSE_1ST_ERR,
	XML_PARSE_2ND,
};

int Kml2Maplus( UCHAR *szSrc, UCHAR *szDst ){
	
	FILE *fpIn;
	FILE *fpOut;
	
	UINT	uFolder	= 0;
	
	UINT	uIcon;
	UINT	uMode = 0;
	UINT	uXmlParse = XML_PARSE_1ST;
	
	MAPLUS_SPOT	*pSpot;
	
	double	dLati, dLong;
	
	UCHAR	*p, *pp;
	UINT	u, uLen;
	
	cordinate_t	Pos;
	
	// spot 数が50 を超えたら，今のフォルダ名を次のフォルダ名にコピーするフラグ
	BOOL	bCopyFolderName = FALSE;
	
	if(( fpIn = fopen( szSrc, "r" )) == NULL ){
		printf( "can't open file \"%s\"\n", szSrc );
		return( 1 );
	}
	
	/*** DAT 初期化 ***/
	bzero( g_pData, sizeof( MAPLUS_DATA ));
	strcpy( g_pData->cHeader, "0100" );
	
	/*** config リード ***/
	GetCfgIconTable();
	
	while( fgets( g_szBuf, BUF_SIZE, fpIn )){
		
		*g_szFolder = *g_szSpot = *g_szComment = '\0';
		
		/*** キーワードサーチ ***/
		
		for( u = 0; u < KML_LAST; ++u ){
			if(( p = strstr( g_szBuf, g_szKmlKeyword[ u ] )) != NULL ) break;
		}
		
		if( u >= KML_LAST ) continue;
		
		p += strlen( g_szKmlKeyword[ u ] );
		if( pp = strstr( g_szBuf, "</" )) *pp = '\0';
		
		/*** キーワードごとの処理 ***/
		
		switch( u ){
		  case KML_NAME:
			/* フォルダ名 or Spot名 */
			uLen = MultiByteToWideChar(
				CP_UTF8,					// UINT		CodePage,		// コードページ
				0,							// DWORD	dwFlags,		// 文字の種類を指定するフラグ
				p,							// LPCSTR	lpMultiByteStr,	// マップ元文字列のアドレス
				strlen( p ),				// int		cchMultiByte,	// マップ元文字列のバイト数
				( WCHAR *)g_szFolder,		// LPWSTR	lpWideCharStr,	// マップ先ワイド文字列を入れるバッファのアドレス
				sizeof( g_szFolder )		// int		cchWideChar		// バッファのサイズ
			);
			(( WCHAR *)g_szFolder )[ uLen ] = 0;
			
			if( uMode == MODE_SPOT ){
				ConvHan2Zen( pSpot->szName, ( WCHAR *)g_szFolder );
			}else{
				bzero( g_pData->Folder[ uFolder ].szName, sizeof( g_pData->Folder[ uFolder ].szName ));
				ConvHan2Zen( g_pData->Folder[ uFolder ].szName, ( WCHAR *)g_szFolder );
				bCopyFolderName = FALSE;
			}
			
		  Case KML_FOLDER:
			/* フォルダ */
			
			if( uXmlParse == XML_PARSE_1ST_ERR ){
				// リパース要求が出ていたら，先頭からもう一度スキャン
				fseek( fpIn, 0, SEEK_SET );
				uXmlParse = XML_PARSE_2ND;
				break;
			}
			
			uMode = MODE_FOLDER;
			
		  Case KML_FOLDER2:
			/* フォルダ脱出 */
			if(
				g_pData->Folder[ uFolder ].uNum != 0 &&
				++uFolder >= MAX_FOLDER_NUM
			) goto ExitLoop;
			
		  Case KML_PLACEMARK:
			/* spot */
			pSpot = &g_pData->Folder[ uFolder ].Spot[ g_pData->Folder[ uFolder ].uNum ];
			uMode = MODE_SPOT;
			
			if( bCopyFolderName ){
				// strcpy の代わり
				ConvHan2Zen(
					g_pData->Folder[ uFolder ].szName,
					g_pData->Folder[ uFolder - 1 ].szName
				);
				bCopyFolderName = FALSE;
			}
			
		  Case KML_PLACEMARK2:
			/* spot脱出 */
			// WGS84->Tokyo 変換
			Pos.lat = dLati;
			Pos.lon = dLong;
			Pos.alt = 0;
			Molodensky( &Pos, &mTokyo );
			pSpot->uLati	= ( UINT )( Pos.lat * 0x60000 + .5 );
			pSpot->uLong	= ( UINT )( Pos.lon * 0x40000 + .5 );
			/*
			pSpot->uLati	= ( UINT )(( dLati + 0.00010696  * dLati - 0.000017467 * dLong - 0.0046020 ) * 0x60000 + .5 );
			pSpot->uLong	= ( UINT )(( dLong + 0.000046047 * dLati + 0.000083049 * dLong - 0.010041  ) * 0x40000 + .5 );
			*/
			
			uMode = MODE_FOLDER;
			if( ++g_pData->Folder[ uFolder ].uNum >= MAX_SPOT_NUM ){
				if( ++uFolder >= MAX_FOLDER_NUM ) goto ExitLoop;
				bCopyFolderName = TRUE;
			}
			
		  Case KML_DESCR:
			/* コメント */
			if( pp = strchr( p, 0xD )) *pp = 0;
			if( pp = strchr( p, 0xA )) *pp = 0;
			
			uLen = MultiByteToWideChar(
				CP_UTF8,					// UINT		CodePage,		// コードページ
				0,							// DWORD	dwFlags,		// 文字の種類を指定するフラグ
				p,							// LPCSTR	lpMultiByteStr,	// マップ元文字列のアドレス
				strlen( p ),				// int		cchMultiByte,	// マップ元文字列のバイト数
				( WCHAR *)g_szComment,			// LPWSTR	lpWideCharStr,	// マップ先ワイド文字列を入れるバッファのアドレス
				sizeof( g_szComment )			// int		cchWideChar		// バッファのサイズ
			);
			(( WCHAR *)g_szComment )[ uLen ] = 0;
			
			ConvHan2Zen( pSpot->szComment, ( WCHAR *)g_szComment );
			
		  Case KML_HREF:
		  case KML_STYLE_URL:
			/* アイコン URL */
			
			// style id の定義なら，style_id->アイコン# を Tbl に登録
			if( uMode == MODE_STYLE_ID ){
				if( uXmlParse == XML_PARSE_2ND ){
					// 2回目のパースなら，見つからなくてもデフォルトを登録
					AddIcon( GetIconID( p, 0 ), g_pIconPathBufTail );
					
					DebugMsg( "2ND:%u:%s->%s\n",
						g_IconTbl[ g_uIconNum - 1 ].uID,
						g_IconTbl[ g_uIconNum - 1 ].szPath,
						p
					);
					
				}else if(( u = GetIconID( p, 1 )) != ICON_NONE ){
					// 1回目でアイコンが見つかった
					AddIcon( u, g_pIconPathBufTail );
					
					DebugMsg( "1ST:%u:%s->%s\n",
						g_IconTbl[ g_uIconNum - 1 ].uID,
						g_IconTbl[ g_uIconNum - 1 ].szPath,
						p
					);
					
				}else{
					// 1回目でアイコンが見つからなかった -- リパースモードに移行
					uXmlParse = XML_PARSE_1ST_ERR;
					
					DebugMsg( "1ST:none:%s\n", p );
				}
				
				uMode = MODE_NONE;
				break;
			}
			
			/* アイコン */
			if( uMode == MODE_SPOT ) pSpot->uIcon	= GetIconID( p, 0 );
			
		  Case KML_STYLE_ID:
		  case KML_STYLEMAP_ID:
			/* スタイルID定義 */
			g_IconTbl[ g_uIconNum ].szPath	= p;
			
			strcpy( g_pIconPathBufTail, "#" );
			strcat( g_pIconPathBufTail + 1, p );
			if( p = strrchr( g_pIconPathBufTail, '"' )) *p = '\0';
			
			uMode = MODE_STYLE_ID;
			
		//  Case KML_LONGITUDE:
		//	/* 東経 */
		//	dLong = atof( p );
		//	
		//  Case KML_LATITUDE:
		//	/* 北緯 */
		//	dLati = atof( p );
			
		  Case KML_COORDINATES:
			dLong = atof( p );
			p = strchr( p, ',' ) + 1;
			dLati = atof( p );
		}
	}
	
  ExitLoop:
	// FAVORITE.DAT 書き出し
	if(( fpOut = fopen( szDst, "wb" )) == NULL ){
		printf( "can't open file \"%s\"\n", szDst );
		return( 1 );
	}
	fwrite( g_pData, sizeof( MAPLUS_DATA ), 1, fpOut );
	fclose( fpOut );
	
	return( 0 );
}

/*** maplus → kml **********************************************************/

int Maplus2Kml( UCHAR *szSrc, UCHAR *szDst ){
	
	FILE *fpIn;
	FILE *fpOut;
	
	UINT	uFolder;
	UINT	uSpot;
	
	double	dLati, dLong;
	double	dLati2, dLong2;
	
	cordinate_t Pos;
	
	// FAVORITE.DAT リード
	if(( fpIn = fopen( szSrc, "rb" )) == NULL ){
		printf( "can't open file \"%s\"\n", szSrc );
		return( 1 );
	}
	
	fread( g_pData, sizeof( MAPLUS_DATA ), 1, fpIn );
	fclose( fpIn );
	
	if( strcmp( g_pData->cHeader, "0100" ) != 0 ){
		puts( "not MAPLUS FAVORITE.DAT" );
		return( 1 );
	}
	
	/*** config リード ***/
	
	if( !(
		GetCfgAllData( g_szFolderHeader, sizeof( g_szFolderHeader ), "[kml_folder_header]" ) &&
		GetCfgAllData( g_szFolderFooter, sizeof( g_szFolderFooter ), "[kml_folder_footer]" ) &&
		GetCfgAllData( g_szSpotBody,	 sizeof( g_szSpotBody ),	 "[kml_spot]" )
	)) return( 1 );
	
	GetCfgIconTable();
	
	/*** 出力ファイル open ***/
	
	if(( fpOut = fopen( szDst, "wb" )) == NULL ){
		printf( "can't open file \"%s\"\n", szDst );
		return( 1 );
	}
	
	/*** kml ヘッダ出力 ***/
	
	if( SearchCfgKey( g_szBuf, BUF_SIZE, "[kml_header]" ) == NULL ) return( 1 );
	
	while( fgets( g_szBuf, BUF_SIZE, g_fpCfg ) && *g_szBuf != '[' ){
		fputs( g_szBuf, fpOut );
	}
	
	/*** header 読み飛ばし ***/
	
	for( uFolder = 0; uFolder < MAX_FOLDER_NUM; ++uFolder ){
		
		if( g_pData->Folder[ uFolder ].uNum == 0 ) continue;
		
		WideCharToMultiByte(
			CP_UTF8,			// UINT		uCodePage,		// コードページ
			0,					// DWORD	dwFlags,		// フラグ
			( LPCWSTR )g_pData->Folder[ uFolder ].szName,
								// PCWSTR	pWideCharStr,	// 変換元の文字列アドレス
			sizeof( g_pData->Folder[ uFolder ].szName ) / sizeof( WCHAR ),
								// int		cchWideChar,	// 文字列の長さ
			g_szFolder,			// PSTR		pMultiByteStr,	// バッファアドレス
			BUF_SIZE,			// int		cchMultiByte,	// 文字列の長さ
			NULL,				// PCSTR	pDefaultChar,	// デフォルトキャラクタ
			NULL				// PBOOL	pUsedDefaultChar// フラグを格納するアドレス
		);
		
		/*** フォルダヘッダ出力 ***/
		
		fprintf( fpOut, g_szFolderHeader, g_szFolder );
		
		/***/
		
		for( uSpot = 0; uSpot < g_pData->Folder[ uFolder ].uNum && uSpot < MAX_SPOT_NUM; ++uSpot ){
			/*** spot 読み込み ***/
			
			// 名前
			WideCharToMultiByte(
				CP_UTF8,			// UINT		uCodePage,		// コードページ
				0,					// DWORD	dwFlags,		// フラグ
				( LPCWSTR )g_pData->Folder[ uFolder ].Spot[ uSpot ].szName,
									// PCWSTR	pWideCharStr,	// 変換元の文字列アドレス
				sizeof( g_pData->Folder[ uFolder ].Spot[ uSpot ].szName ) / sizeof( WCHAR ),
									// int		cchWideChar,	// 文字列の長さ
				g_szSpot,			// PSTR		pMultiByteStr,	// バッファアドレス
				BUF_SIZE,			// int		cchMultiByte,	// 文字列の長さ
				NULL,				// PCSTR	pDefaultChar,	// デフォルトキャラクタ
				NULL				// PBOOL	pUsedDefaultChar// フラグを格納するアドレス
			);
			
			// アイコン, 座標
			dLati = ( double )g_pData->Folder[ uFolder ].Spot[ uSpot ].uLati / 0x60000;
			dLong = ( double )g_pData->Folder[ uFolder ].Spot[ uSpot ].uLong / 0x40000;
			// Tokyo->WGS84 変換
			
			// Tokyo で高度 0m になる WGS84 高度を求める
			/*
			Pos.lat = dLati;
			Pos.lon = dLong;
			Pos.alt = 0;
			Molodensky( &Pos, &mTokyo );
			*/
			Pos.alt = 0;
			
			Pos.lat = dLati;
			Pos.lon = dLong;
			Molodensky( &Pos, &mWGS );
			dLati2 = Pos.lat;
			dLong2 = Pos.lon;
			/*
			dLati2	= dLati - 0.00010695  * dLati + 0.000017464 * dLong + 0.0046017;
			dLong2	= dLong - 0.000046038 * dLati - 0.000083043 * dLong + 0.010040;
			*/
			
			// コメント
			WideCharToMultiByte(
				CP_UTF8,			// UINT		uCodePage,		// コードページ
				0,					// DWORD	dwFlags,		// フラグ
				( LPCWSTR )&g_pData->Folder[ uFolder ].Spot[ uSpot ].szComment,
									// PCWSTR	pWideCharStr,	// 変換元の文字列アドレス
				sizeof( g_pData->Folder[ uFolder ].Spot[ uSpot ].szComment ) / sizeof( WCHAR ),
									// int		cchWideChar,	// 文字列の長さ
				g_szComment,		// PSTR		pMultiByteStr,	// バッファアドレス
				BUF_SIZE,			// int		cchMultiByte,	// 文字列の長さ
				NULL,				// PCSTR	pDefaultChar,	// デフォルトキャラクタ
				NULL				// PBOOL	pUsedDefaultChar// フラグを格納するアドレス
			);
			
			fprintf( fpOut,
				g_szSpotBody,	// format string
				g_szSpot,
				g_szComment,
				dLong2, dLati2,
				GetIconPath( g_pData->Folder[ uFolder ].Spot[ uSpot ].uIcon ),
				dLong2, dLati2
			);
		}
		
		/*** フォルダフッタ出力 ***/
		
		fprintf( fpOut, g_szFolderFooter );
	}
	
	/*** kml フッタ出力 ***/
	if( SearchCfgKey( g_szBuf, BUF_SIZE, "[kml_footer]" ) == NULL ) return( 1 );
	
	while( fgets( g_szBuf, BUF_SIZE, g_fpCfg ) && *g_szBuf != '[' ){
		fputs( g_szBuf, fpOut );
	}
	
	return( 0 );
}

/*** txt → maplus **********************************************************/

int Txt2Maplus( UCHAR *szSrc, UCHAR *szDst, UINT uInputMode ){
	
	FILE *fpIn;
	FILE *fpOut;
	
	UINT	uFolder	= 0;
	
	UINT	uLatiD, uLatiM;
	double	dLati;
	UINT	uLongD, uLongM;
	double	dLong;
	UINT	uIcon;
	
	MAPLUS_SPOT	*pSpot;
	
	UCHAR	*p;
	
	if(( fpIn = fopen( szSrc, "r" )) == NULL ){
		printf( "can't open file \"%s\"\n", szSrc );
		return( 1 );
	}
	
	/*** DAT 初期化 ***/
	bzero( g_pData, sizeof( MAPLUS_DATA ));
	strcpy( g_pData->cHeader, "0100" );
	
	while( fgets( g_szBuf, BUF_SIZE, fpIn )){
		
		*g_szFolder = *g_szSpot = *g_szComment = '\0';
		
		if( uInputMode == MODE_CSV ) ConvCsv2Tab( g_szBuf );
		
		sscanf(
			g_szBuf,
			"%1023[^\t]\t"
			"%1023[^\t]\t"
			"%d\t"
			"%127[^\t]\t"
			"%127[^\t]\t"
			"%1023[^\t]",
			g_szFolder,
			g_szSpot,
			&uIcon,
			g_szLati,
			g_szLong,
			g_szComment
		);
		
		if(( p = strchr( g_szComment, 0xD ))) *p = '\0';
		if(( p = strchr( g_szComment, 0xA ))) *p = '\0';
		
		// フォルダ名が違う，または spot が MAX_SPOT_NUM に達したら
		// 次の folder へ
		
		if(
			strcmp( g_szFolder, g_szPrevFolder ) != 0 && g_pData->Folder[ uFolder ].uNum != 0 ||
			g_pData->Folder[ uFolder ].uNum == MAX_SPOT_NUM
		){
			if( ++uFolder >= MAX_FOLDER_NUM ){
				printf( "folder num > %d\n", MAX_FOLDER_NUM );
				break;
			}
		}
		
		if( g_pData->Folder[ uFolder ].uNum == 0 ){
			MultiByteToWideChar(
				CP_ACP,						// UINT		CodePage,		// コードページ
				0,							// DWORD	dwFlags,		// 文字の種類を指定するフラグ
				g_szFolder,					// LPCSTR	lpMultiByteStr,	// マップ元文字列のアドレス
				strlen( g_szFolder ),		// int		cchMultiByte,	// マップ元文字列のバイト数
				( WCHAR *)g_pData->Folder[ uFolder ].szName,	// LPWSTR	lpWideCharStr,	// マップ先ワイド文字列を入れるバッファのアドレス
				sizeof( g_pData->Folder[ uFolder ].szName )		// int		cchWideChar		// バッファのサイズ
			);
			
			ConvHan2Zen( g_pData->Folder[ uFolder ].szName, g_pData->Folder[ uFolder ].szName );
			
			strcpy( g_szPrevFolder, g_szFolder );
		}
		
		/*** Spot 出力 ***/
		
		pSpot = &g_pData->Folder[ uFolder ].Spot[ g_pData->Folder[ uFolder ].uNum ];
		
		MultiByteToWideChar(
			CP_ACP,						// UINT		CodePage,		// コードページ
			0,							// DWORD	dwFlags,		// 文字の種類を指定するフラグ
			g_szSpot,					// LPCSTR	lpMultiByteStr,	// マップ元文字列のアドレス
			strlen( g_szSpot ),			// int		cchMultiByte,	// マップ元文字列のバイト数
			( WCHAR *)pSpot->szName,	// LPWSTR	lpWideCharStr,	// マップ先ワイド文字列を入れるバッファのアドレス
			sizeof( pSpot->szName )		// int		cchWideChar		// バッファのサイズ
		);
		
		MultiByteToWideChar(
			CP_ACP,						// UINT		CodePage,		// コードページ
			0,							// DWORD	dwFlags,		// 文字の種類を指定するフラグ
			g_szComment,				// LPCSTR	lpMultiByteStr,	// マップ元文字列のアドレス
			strlen( g_szComment ),		// int		cchMultiByte,	// マップ元文字列のバイト数
			( WCHAR *)pSpot->szComment,// LPWSTR	lpWideCharStr,	// マップ先ワイド文字列を入れるバッファのアドレス
			sizeof( pSpot->szComment )// int		cchWideChar		// バッファのサイズ
		);
		
		ConvHan2Zen( pSpot->szName,    pSpot->szName );
		ConvHan2Zen( pSpot->szComment, pSpot->szComment );
		
		// 緯度・経度
		if( sscanf( g_szLati, "%d%*[^0-9]%d%*[^0-9]%lf", &uLatiD, &uLatiM, &dLati ) < 3 ){
			uLatiD = uLatiM = 0;
			dLati = atof( g_szLati ) * 3600;
		}
		if( sscanf( g_szLong, "%d%*[^0-9]%d%*[^0-9]%lf", &uLongD, &uLongM, &dLong ) < 3 ){
			uLongD = uLongM = 0;
			dLong = atof( g_szLong ) * 3600;
		}
		
		pSpot->uIcon	= uIcon;
		pSpot->uLati	= ( UINT )(( double )( uLatiD * 3600 + uLatiM * 60 + dLati ) * 0x60000 / 3600 + .5 );
		pSpot->uLong	= ( UINT )(( double )( uLongD * 3600 + uLongM * 60 + dLong ) * 0x40000 / 3600 + .5 );
		
		++g_pData->Folder[ uFolder ].uNum;
	}
	
	// FAVORITE.DAT 書き出し
	if(( fpOut = fopen( szDst, "wb" )) == NULL ){
		printf( "can't open file \"%s\"\n", szDst );
		return( 1 );
	}
	fwrite( g_pData, sizeof( MAPLUS_DATA ), 1, fpOut );
	fclose( fpOut );
	
	return( 0 );
}

/*** maplus → txt **********************************************************/

int Maplus2Txt( UCHAR *szSrc, UCHAR *szDst, UINT uOutputMode ){
	
	FILE *fpIn;
	FILE *fpOut;
	
	UINT	uFolder;
	UINT	uSpot;
	
	double	dLati, dLong;
	UCHAR	*p;
	
	// FAVORITE.DAT リード
	if(( fpIn = fopen( szSrc, "rb" )) == NULL ){
		printf( "can't open file \"%s\"\n", szSrc );
		return( 1 );
	}
	
	fread( g_pData, sizeof( MAPLUS_DATA ), 1, fpIn );
	fclose( fpIn );
	
	if( strcmp( g_pData->cHeader, "0100" ) != 0 ){
		puts( "not MAPLUS FAVORITE.DAT" );
		return( 1 );
	}
	
	if( GetCfgData( g_szDegFormat, sizeof( g_szDegFormat ), "[degree_format]" ) == NULL ) return( 1 );
	
	/***/
	
	if(( fpOut = fopen( szDst, "w" )) == NULL ){
		printf( "can't open file \"%s\"\n", szDst );
		return( 1 );
	}
	
	/*** header 読み飛ばし ***/
	
	for( uFolder = 0; uFolder < MAX_FOLDER_NUM; ++uFolder ){
		
		WideCharToMultiByte(
			CP_ACP,				// UINT		uCodePage,		// コードページ
			0,					// DWORD	dwFlags,		// フラグ
			( LPCWSTR )g_pData->Folder[ uFolder ].szName,
								// PCWSTR	pWideCharStr,	// 変換元の文字列アドレス
			sizeof( g_pData->Folder[ uFolder ].szName ) / sizeof( WCHAR ),
								// int		cchWideChar,	// 文字列の長さ
			g_szFolder,			// PSTR		pMultiByteStr,	// バッファアドレス
			BUF_SIZE,			// int		cchMultiByte,	// 文字列の長さ
			NULL,				// PCSTR	pDefaultChar,	// デフォルトキャラクタ
			NULL				// PBOOL	pUsedDefaultChar// フラグを格納するアドレス
		);
		
		for( uSpot = 0; uSpot < g_pData->Folder[ uFolder ].uNum && uSpot < MAX_SPOT_NUM; ++uSpot ){
			p = g_szBuf2;
			
			/*** spot 読み込み ***/
			
			// 名前
			WideCharToMultiByte(
				CP_ACP,				// UINT		uCodePage,		// コードページ
				0,					// DWORD	dwFlags,		// フラグ
				( LPCWSTR )g_pData->Folder[ uFolder ].Spot[ uSpot ].szName,
									// PCWSTR	pWideCharStr,	// 変換元の文字列アドレス
				sizeof( g_pData->Folder[ uFolder ].Spot[ uSpot ].szName ) / sizeof( WCHAR ),
									// int		cchWideChar,	// 文字列の長さ
				g_szBuf,			// PSTR		pMultiByteStr,	// バッファアドレス
				BUF_SIZE,			// int		cchMultiByte,	// 文字列の長さ
				NULL,				// PCSTR	pDefaultChar,	// デフォルトキャラクタ
				NULL				// PBOOL	pUsedDefaultChar// フラグを格納するアドレス
			);
			
			sprintf( p, "%s\t%s\t", g_szFolder, g_szBuf );
			p = strchr( p, '\0' );
			
			// アイコン, 座標
			dLati = ( double )g_pData->Folder[ uFolder ].Spot[ uSpot ].uLati * 3600 / 0x60000;
			dLong = ( double )g_pData->Folder[ uFolder ].Spot[ uSpot ].uLong * 3600 / 0x40000;
			
			sprintf(
				g_szLati, g_szDegFormat,
				( UINT )dLati / 3600,
				( UINT )dLati % 3600 / 60,
				dLati - ( UINT )( dLati / 60 ) * 60
			);
			
			sprintf(
				g_szLong, g_szDegFormat,
				( UINT )dLong / 3600,
				( UINT )dLong % 3600 / 60,
				dLong - ( UINT )( dLong / 60 ) * 60
			);
			
			sprintf(
				p,
				"%d\t%s\t%s\t",
				g_pData->Folder[ uFolder ].Spot[ uSpot ].uIcon,
				g_szLati, g_szLong
			);
			p = strchr( p, '\0' );
			
			// コメント
			WideCharToMultiByte(
				CP_ACP,				// UINT		uCodePage,		// コードページ
				0,					// DWORD	dwFlags,		// フラグ
				( LPCWSTR )&g_pData->Folder[ uFolder ].Spot[ uSpot ].szComment,
									// PCWSTR	pWideCharStr,	// 変換元の文字列アドレス
				sizeof( g_pData->Folder[ uFolder ].Spot[ uSpot ].szComment ) / sizeof( WCHAR ),
									// int		cchWideChar,	// 文字列の長さ
				g_szBuf,			// PSTR		pMultiByteStr,	// バッファアドレス
				BUF_SIZE,			// int		cchMultiByte,	// 文字列の長さ
				NULL,				// PCSTR	pDefaultChar,	// デフォルトキャラクタ
				NULL				// PBOOL	pUsedDefaultChar// フラグを格納するアドレス
			);
			
			sprintf( p, "%s\n", g_szBuf );
			p = strchr( p, '\0' );
			
			if( uOutputMode == MODE_CSV ){
				fputs( ConvTab2Csv( g_szBuf2 ), fpOut );
			}else{
				fputs( g_szBuf2, fpOut );
			}
		}
	}
	
	return( 0 );
}

/*** main *******************************************************************/

int main( int argc, UCHAR **argv ){
	
	static UCHAR szDst[ MAX_PATH ];
	UINT	uOutputMode = MODE_TXT;
	UCHAR	*p;
	
	*g_szPrevFolder = '\0';
	
	StrTokFile( szDst, argv[ 0 ], STF_NODE );
	
	if( argc <= 1 ){
		printf( "usage: %s <src_file> [<dst_file>]\n", szDst );
		return( 0 );
	}
	
	/*** config ファイルリード **********************************************/
	
	if(( g_fpCfg = fopen( ChangeExt( szDst, argv[ 0 ], "cfg" ), "r" )) == NULL ){
		printf( "can't open file \"%s\"\n", szDst );
		return( 1 );
	}
	
	// default output
	if( GetCfgData( g_szBuf, BUF_SIZE, "[output]" )){
		uOutputMode =	stricmp( g_szBuf, "kml" ) == 0 ? MODE_KML :
						stricmp( g_szBuf, "csv" ) == 0 ? MODE_CSV : MODE_TXT;
	}
	
	DebugMsg( "output:%s\n", g_szBuf );
	
	/************************************************************************/
	
	if(
		( g_pData = malloc( sizeof( MAPLUS_DATA ))) == NULL ||
		( g_pIconPathBufTail = g_pIconPathBuf = malloc( BUF_SIZE_ICON_PATH )) == NULL
	){
		puts( "not enough memony" );
	}
	
	return
		IsExt( argv[ 1 ], "dat" ) ? (
			uOutputMode == MODE_KML ? Maplus2Kml( argv[ 1 ], argc >= 3 ? argv[ 2 ] : ChangeExt( szDst, argv[ 1 ], "kml" )) :
			uOutputMode == MODE_CSV ? Maplus2Txt( argv[ 1 ], argc >= 3 ? argv[ 2 ] : ChangeExt( szDst, argv[ 1 ], "csv" ), uOutputMode ) :
									  Maplus2Txt( argv[ 1 ], argc >= 3 ? argv[ 2 ] : ChangeExt( szDst, argv[ 1 ], "txt" ), uOutputMode )
		) :
		IsExt( argv[ 1 ], "kml" ) ? Kml2Maplus( argv[ 1 ], argc >= 3 ? argv[ 2 ] : ChangeExt( szDst, argv[ 1 ], "dat" )) :
		IsExt( argv[ 1 ], "csv" ) ? Txt2Maplus( argv[ 1 ], argc >= 3 ? argv[ 2 ] : ChangeExt( szDst, argv[ 1 ], "dat" ), MODE_CSV ) :
									Txt2Maplus( argv[ 1 ], argc >= 3 ? argv[ 2 ] : ChangeExt( szDst, argv[ 1 ], "dat" ), MODE_TXT );
}
