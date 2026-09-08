1 BN$="PKGAME"   :REM DEFAULT BASENAME FOR THIS SAVE BUNDLE
2 MF$=BN$+".MAP" :REM STATIC WORLD MAP
3 SF$=BN$+".SAV" :REM DYNAMIC GAME STATE
4 GF$="PKX16"    :REM GAME PROGRAM
5 MB=1:MA=$A000  :REM MAP START BANK/ADDR (17 BANKS!)
6 SB=20:SA=$A000  :REM SAVE START BANK/ADDR

7 REM CONTRACT: LOAD MAP + SAVE INTO BANKED RAM, THEN LOAD PROGRAM.
8 REM ASSUME LOAD SPILLS INTO HIGHER BANKS WHEN NEEDED.
9 REM VERIFY SPILLOVER ON REAL X16/EMULATOR BEFORE DEPENDING ON IT.

10 COLOR 0,0 : ? CHR$($93) : REM BLANK SCREEN
11 COLOR 2,0
12 ? CHR$($93):?:?:?

25 REM FONT IS "CALIGRAPHY"

30?"        ***** **                                           
31?"     ******  ****     *                        *           
32?"    **   *  *  ***   ***                      **           
33?"   *    *  *    ***   *                       **           
34?"       *  *      **      ***  ****          ********       
35?"      ** **      ** ***   **** **** * **** ******** ***    
36?"      ** **      **  ***   **   **** * ***  * **   * ***   
37?"    **** **      *    **   **       *   ****  **  *   ***  
38?"   * *** **     *     **   **      **    **   ** **    *** 
39?"      ** *******      **   **      **    **   ** ********  
41?"      ** ******       **   **      **    **   ** *******   
42?"      ** **           **   **      **    **   ** **        
43?"      ** **           **   ***     **    **   ** ****    * 
44?"      ** **           *** * ***     ***** **   ** *******  
45?" **   ** **            ***           ***   **      *****   
46?"***   *  *  
47?" ***    *   
48?"  ******    
49?"    ***     
50?"            
51?"            
52?"     *****                                 **  
53?"  ******             *                      ** 
54?" **   *  *    *     ***                     ** 
55?"*    *  *   *** *    *                      ** 
56?"    *  *     ***                            **                         **  
57?"   ** **    * *    ***  **  **   ***     ** **    ***  ****** ***     * ** 
58?"   ** **   *        ***  ****** * *** * ******** *  *** ** *** **  * ** ***
59?"   ** *****          **   *  ***   *** **  **** **   *   *  *** *** ****   
60?"   ** ** ***         **   *   * *   ** **   **  **   *   *   *   *    **   
61?"   ** **   ***       **   *   * *   ** **   **  **   *   *   *   *      ** 
62?"   *  **    **       **   *   * *   ** **   **  **   *   *   *   *       **
63?"      *      **      **   *   * *   ** **   **  **   *   *   *   *   ***  *
64?"  ****        **     **   *   * *   ** **   **   ****    *   *   *  * *** *
65?" *  *****      **  * ** * **  ** *****  ****      ***    **  **  **    *** 
66?"*    ***        ***   **  **  ***** **   **               **  **  **       
67?"*                                    **   
68?" **                              **   **  
69?"                                ****  *   
70?"                               *  ****    
100 FORX=1 TO 80:L$=L$+CHR$(164):NEXT
110 ?:?:?:?:? L$;
120 COLOR 7
130 ?:?:? SPC(27);"PRESS ANY KEY TO BEGIN"
140 GET A$: IF A$="" GOTO 140
150 POKE 0,MB                    :REM LOAD MAP INTO BANKED RAM
160 LOAD MF$,8,1,MA
170 POKE 0,SB                    :REM LOAD SAVE STATE INTO BANKED RAM
180 LOAD SF$,8,1,SA
190 POKE 0,0                     :REM RESTORE DEFAULT BANK
200 LOAD GF$,8,1                 :REM LOAD GAME BINARY
