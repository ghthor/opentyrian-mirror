#include "shpmast.h"

const JE_string shpfile[SHPnum] = /* [1..SHPnum] */
{
    "ESTFE.SHP", /*2*/
    "ESTFD.SHP", /*3*/
    "ESTFC.SHP", /*6*/
    "ESTPA.SHP", /*1*/
    "ESTFF.SHP", /*4*/
    "ESTSA.SHP", /*5*/
    "ESTWB.SHP", /*7*/
    "NEWSH(.SHP",
    "NEWSH{.SHP",
    "NEWSH&.SHP",
    "NEWSHQ.SHP",
    "NEWSH`.SHP"
};

/*

===================
TYPE 5: Shape Files
===================
      SHAPES1.DAT  o - - -  Items
      SHAPES3.DAT  o - - -  Shots
      SHAPES5.DAT  o - - -  Two-Player Stuff
      SHAPES6.DAT  o - - -  Explosions
      SHAPES9.DAT  o - - -  Player ships/options

 1    SHAPES2.DAT  - o - -  Tyrian ships
 2    SHAPES4.DAT  - o - -  TyrianBoss
 3    SHAPES7.DAT  - - - -  Iceships
 4    SHAPES8.DAT  - - - -  Tunnel World
 5    SHAPESA.DAT  o - - -  Mine Stuff
 6    SHAPESB.DAT  - - - -  IceBoss
 7    SHAPESC.DAT  - o - -  Deliani Stuff
 8    SHAPESD.DAT  o - - -  Asteroid Stuff I
 9    SHAPESE.DAT  - o - -  Tyrian Bonus Rock + Bubbles
 10   SHAPESF.DAT  - o - -  Savara Stuff I
 11   SHAPESG.DAT  - - - -  Giger Stuff
 12   SHAPESH.DAT  - - - -  Giger Stuff
 13   SHAPESI.DAT  - o - -  Savara Stuff II
 14   SHAPESJ.DAT  - - - -  Jungle Stuff
 15   SHAPESK.DAT  - - - -  Snowballs
 16   SHAPESL.DAT  - o - -  Savara Boss
 17   SHAPESM.DAT  o - - -  Asteroid Stuff IV
 18   SHAPESN.DAT  - - - -  Giger Boss
 19   SHAPESO.DAT  - o - -  Savara Boss
 20   SHAPESP.DAT  o - - -  Asteroid Stuff III
 21   SHAPESQ.DAT  o - - -  Coins and Gems
 22   SHAPESR.DAT  - - - -  TunnelWorld Boss
 23   SHAPESS.DAT  o - - -  Asteroid Stuff II
 24   SHAPEST.DAT  - o - -  Deli Boss
 25   SHAPESU.DAT  - - - -  Deli Stuff II
 26   SHAPESV.DAT  - - - -  Jungle Stuff
 27   SHAPESW.DAT  - - - -  Sawblades

                   M 1 2 3  episode
*/
