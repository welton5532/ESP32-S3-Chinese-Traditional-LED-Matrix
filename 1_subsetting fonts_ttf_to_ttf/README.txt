==============================================================================
             FONT SUBSETTING GUIDE: TAIPEI SANS TC (TTF)
==============================================================================

DIRECTORY STRUCTURE:
.
├── README.txt
├── TaipeiSansTCBeta-Regular.ttf        <-- Source Font (~20MB)
└── tw_edu_common_4808_chars.txt        <-- Text file with Chinese Characters

GOAL:
Create a smaller .ttf file containing only:
1. Basic Latin (English, Numbers, and the SPACE 0x20 character)
2. Ministry of Education 4808 Common Characters
3. Standard Chinese Punctuation (Full-width comma, period, etc.)

------------------------------------------------------------------------------
[ STEP 1: INSTALLATION ]
------------------------------------------------------------------------------
1. Ensure you have Python installed.
2. Open PowerShell or Command Prompt in this folder.
3. Install the fonttools library by running:

   pip install fonttools

------------------------------------------------------------------------------
[ STEP 2: GENERATE SUBSET ]
------------------------------------------------------------------------------
Run the following command in your terminal. 

(Note: We explicitly include Unicode range U+0000-007F to guarantee the 
Space character (0x20) is preserved).

   python -m fontTools.subset "TaipeiSansTCBeta-Regular.ttf" --text-file="tw_edu_common_4808_chars.txt" --unicodes="U+0000-007F,U+3000-303F" --layout-features="*" --output-file="TaipeiSansTC-Subset.ttf"

------------------------------------------------------------------------------
[ PARAMETER EXPLANATION ]
------------------------------------------------------------------------------
--text-file       : Reads the 4808 Chinese characters from your .txt file.
--unicodes        :
      U+0000-007F : Basic Latin. Includes Space (0x20), English, Numbers.
      U+3000-303F : CJK Symbols/Punctuation. Includes full-width Space, 
                    comma (，), period (。), quotes (「」).
--layout-features : Keeps OpenType features (essential for correct vertical 
                    text and punctuation alignment).
--output-file     : The name of your new, smaller font file.

------------------------------------------------------------------------------
[ EXPECTED RESULT ]
------------------------------------------------------------------------------
A new file named "TaipeiSansTC-Subset.ttf" will appear in this folder.
Estimated size: ~2.0 MB - 3.0 MB.