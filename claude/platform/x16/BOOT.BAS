1 MF$="ARCHIPELAGO.MAP" :REM MAP FILE
2 MB = 1                :REM MAP BANK
3 MA = $A000            :REM MAP START ADDRESS
4 GF$="PKX16"           :REM GAME FILE

5 REM PROVISIONAL CONTRACT: LOAD MAP INTO BANKED RAM AT MAPBANK:MAPADDR.
6 REM ASSUME LOAD SPILLS INTO HIGHER BANKS IF THE FILE EXCEEDS ONE BANK.
7 REM VERIFY THAT BEHAVIOR ON REAL X16 / EMULATOR BEFORE ENGINE CODE DEPENDS ON IT.

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
150 POKE 0,MB                 :REM SELECT BANKED RAM START BANK FOR MAP LOAD
160 LOAD MF$,8,1,M            :REM MAP SHOULD CONTINUE INTO HIGHER BANKS IF NEEDED
170 POKE 0,0                  :REM RESTORE DEFAULT BANK BEFORE LOADING MAIN PROGRAM
180 LOAD GF$,8,1              :REM GAME BINARY
