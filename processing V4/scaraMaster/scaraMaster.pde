/*breve introduzione: istruzioni stampate  a schermo,istruzioni : premere I o i per attivare inseguimento, s o S per fermarlo, a o A per ignorare le collisioni,i tasti da 1 a 3 per selezionare i giunti, freccia destra/sinistra per movimento giunto selezionato, tasti su e giù (frecce) per modificare visuale.
Non ho disegnato tutto il piano x z di Processing perchè tanto per come è fatto il progetto l'uniciclo non può uscire dalla porzione di pavimento disegnata, inoltre
l'orientamento con cui l'uniciclo arriva sul punto non è definito precisamente e non è controllabile, ma tuttavia non è quello lo scopo della legge di controllo 
adottata, inoltre c'è una leggera differenza tra posizione desiderata e quella reale ma credo che sia comunque dovuta alla precisione richiesta.Il problema degli angoli è 
e riguarda solo lo sketch dove sto in modalità uniciclo master, qui non ho problemi del genere.
*/

// inizio dichiarazioni variabili
float eyeY = 0;
int inseguimento = 0;
float[] pT= {1,0,0,0};
float[] sT= {0,1,0,0};
float[] tT= {0,0,1,0};
float[] qT= {0,0,0,1};
float xBase;
float yBase;
int segno = 1;
int giunto = 0;
int nGiri = 0;

// dimensioni link 0:
float d0x = 30; // lungo x
float d0y = 200; // lungo y
float d0z = 30; // lungo z

// dimensioni link 1
float d1x = 200; // lungo x
float d1y = 30; // lungo y
float d1z = 30; // lungo z

// dimensioni link 2
float d2x = 200; // lungo x
float d2y = 30; // lungo y
float d2z = 30; // lungo z

// dimensioni link 3
float d3x = 30; // lungo x
float d3y = 200; // lungo y
float d3z = 30; // lungo z

// parametri giunto (theta1, theta2, d3)
float[] theta = {0.7,0,0}; 
float xUniciclo=50;
float yUniciclo=-8+700/2+d0y/2;
float zUniciclo=0;
float xPunto=0;
float yPunto=0;
float zPunto=0;
float kv1 = 0.5;
float kv2 = 0.5;
float thetaUniciclo = 0;
float v1 = 0;
float v2 = 0;
float e_p = 0;
float thetaDes = 0;
int collisione = 0;
int spettralità = 0;
float t1p = 0;
float t2p = 0;
// fine dichiarazione variabili

void setup() 
{
  size(1000, 700, P3D);
  stroke(0);
  strokeWeight(3);
  xBase = width/2;
  yBase = height/2;  
}

 
 
void draw() 
{  
  
// voglio angoli tra -2PI e +2PI per i motivi scritti nella breve intro
 theta[0]=theta[0]%(2*PI);
 theta[1]=theta[1]%(2*PI);
 
// le coordinate della proiezione dello scara su piano x,z di Processing 
 xPunto = xBase+200*cos(theta[0])+150*cos(theta[0]+theta[1]);
 yPunto = yBase;
 zPunto = -200*sin(theta[0])-150*sin(theta[0]+theta[1]);
 
 background(255);
 directionalLight(128, 128, 128, -1, 0, 1);   
 lights();
 //Permette di ruotare la vista:
 camera((width/2.0), height/2 - eyeY, (height/2.0) / tan(PI*60.0 / 360.0), width/2.0, height/2.0, 0, 0, 1, 0);  

  if (keyPressed){
    if((key == 'i') || (key == 'I')) // inseguimento 
      {
        inseguimento = 1;
      }
  }
  if (keyPressed){
    if(key == 's' || key == 'S')   // stop inseguimento
      {
        inseguimento = 0;
      }
  }
  if (keyPressed){
    if(key == 'a' || key == 'A')
      {
        collisione = 0;
        inseguimento = 1;
      }
  }
     if (keyPressed)
  {
    // movimento camera
    if (keyCode == DOWN)
    {
      eyeY -= 5;
    }
    if (keyCode == UP)
    {
      eyeY += 5;
    }

    if (key == '1')
    {
      giunto = 0;
    }
    if (key == '2')
    {
      giunto = 1;
    }
    if (key == '3')
    {
      giunto = 2;
    }
    if (keyCode == LEFT)
    {
      segno = -1;
      muovi();
    }
    if (keyCode == RIGHT)
    {
      segno = 1;
      muovi();      
    }   
  }
  
  // funzione che implementa lo spostamento dell'uniciclo
  if (inseguimento ==1){
      e_p = sqrt(pow(xPunto-xUniciclo,2)+pow(zPunto-zUniciclo,2));
  if (e_p > 1) // mi muovo solo se l'errore è maggiore di una certa quantità
  {
    // assegno velocità secondo legge proporzionale
    v1 = -kv1*((xUniciclo-xPunto)*cos(thetaUniciclo) + (zUniciclo-zPunto)*sin(thetaUniciclo)); 

    // Calcolo l'angolo verso il target: scelgo il multiplo di 2PI 
    // più vicino all'orientamento corrente del robot
    thetaDes = (atan2(zPunto-zUniciclo,xPunto-xUniciclo)+ nGiri*2*PI)%(2*PI);
    if (abs(thetaDes+2*PI-thetaUniciclo) < abs(thetaDes-thetaUniciclo))
    {
      thetaDes = thetaDes+2*PI;
      nGiri += 1;
    }
    else
    {
      if (abs(thetaDes-2*PI-thetaUniciclo) < abs(thetaDes-thetaUniciclo))
      {
        thetaDes = thetaDes-2*PI;
        nGiri += -1;
      }
    }

    // assegno velocità angolare secondo legge proporzionale     
    v2 = kv2*(+thetaDes-thetaUniciclo);
    
  }
  else // se sono già abbastanza vicino al target non mi muovo
  {
    v1 = 0;
    v2 = 0;
  }
      xUniciclo = xUniciclo + v1*cos(thetaUniciclo)*0.01667;
      zUniciclo = zUniciclo + v1*sin(thetaUniciclo)*0.01667;
      thetaUniciclo = thetaUniciclo + v2*0.01667;
   }
   
   
   //rilevatore di collisioni
   
   if ((abs(xUniciclo-xBase)<= 40) && (abs(zUniciclo)<= 40)){
     collisione = 1;
     inseguimento = 0;
   }
   
   // comportamento in caso di collisioni
  if (collisione ==1){
  fill(0,255,0);
     textSize(30);
     text("collisione avvenuta",400,130);
     //uniciclo
     pushMatrix();
     translate(xUniciclo,yUniciclo,zUniciclo);
     fill(255,0,0);
     rotateY(thetaUniciclo);
     box(20,10,10);
     translate(10,-10,0);
     fill(255,255,0);
     box(15,5,5);
     popMatrix();
  }else{
  //uniciclo disegno
  pushMatrix();
  translate(xUniciclo,yUniciclo,zUniciclo);
  fill(100);
  rotateY(-thetaUniciclo);
  box(20,10,10);
  translate(10,-10,0);
  fill(255,255,0);
  box(15,5,5);  // l'orientamento è dato dalla scatola gialla
  popMatrix();
  //fine uniciclo}
  
}
  //pavimentazione , non ho ricoperto tutto il piano x,z di processing per comodità e perchè comunque l'uniciclo non si può muovere fuori dal piano per costruzione
  pushMatrix();    
  translate(0,yBase+d0y/2,-500);
  int k=1;
  for (int i = 0;i<=23;i++){
    for (int j = 0;j<=19;j++){
      if( k==0 ){
          fill(0,200,15);
          box(50,1,50);
      }
      if(k==1){
          fill(42,82,190);
          box(50,1,50);
      }
      translate(0,0,50);
      if(k==1){
        k=0;
      }else{
        k=1;
      }
    }
    translate(50,0,-50*20);
    if(k==1){
        k=0;
      }else{
        k=1;
      }
  } 
  popMatrix();
 //fine pavimentazione
 
 
  pushMatrix(); // Memorizza il sistema di riferimento attuale

  fill(200,0,200); // Colore del robot
  
  // Link 0 (base)
  translate(xBase,yBase);
  box(d0x,d0y,d0z);
  
  // Link 1 (si muove con theta1 = theta[0])
  rotateY(theta[0]); 
  translate((d1x-d0x)/2,-(d0y+d1y)/2,0);
  box(d1x,d1y,d1z);
  
  // Link 2 (si muove con theta2 = theta[1])
  translate((d1x-d2z)/2,0,0);
  rotateY(theta[1]);
  translate((d2x-d2z)/2,0,0);
  box(d2x,d2y,d2z);  
  
  // Link 3 (si muove con d3 = theta[2])
  translate((d2x-d3x)/2,theta[2],0); 
  box(d3x,d3y,d3z);
  
  
  
  popMatrix();  // Ritorna al sistema di riferimento memorizzato
  
  textSize(15);
  fill(255,200,0);
 
 
  //proiezione punto da inseguire
  pushMatrix();
  translate(xBase,yBase);
  rotateY(theta[0]); 
  translate((d1x-d0x)/2,-(d0y+d1y)/2,0); 
  translate((d1x-d2z)/2,0,0);
  rotateY(theta[1]);
  translate(85,0,0);
  translate(85,0,0); 
  translate(0,210,0);
  fill(255,0,0);
  box(20,1,20);
  popMatrix();
  //fine proiezione punto da seguire
  
  // variabili di comodità per avere angoli tra 180° e -180°, nella visualizzazione a schermo soltanto
  if(abs(theta[0]*180/PI)<=180){
    t1p = theta[0]*180/PI;
  } else{
    if ((theta[0]*180/PI)>180){
      t1p = -360+(theta[0]*180/PI);
    }
    if((theta[0]*180/PI)<180){
      t1p = 360+(theta[0]*180/PI);
    }
  }
  
  if(abs(theta[1]*180/PI)<=180){
    t2p = theta[1]*180/PI;
  } else{
    if ((theta[1]*180/PI)>180){
      t2p = -360+(theta[1]*180/PI);
    }
    if((theta[1]*180/PI)<180){
      t2p = 360+(theta[1]*180/PI);
    }
  }
  
  // vari testi e scritte 
  
  textSize(15);
  fill(0);
  text ("istruzioni : premere I o i per attivare inseguimento, s o S per fermarlo, a o A per ignorare le collisioni, ",20,10);
  text("i tasti da 1 a 3 per selezionare i giunti, freccia destra/sinistra per movimento giunto selezionato, tasti su e giù (frecce) per modificare visuale",20,30);
  text("valori variabili di giunto in sequenza:",20,50);
  text("theta 1 =",270,50);
  text(t1p,340,50);
  text("theta 2 = ",400,50);
  text(t2p,470,50);
  text("d3 = ",540,50);
  text(theta[2]+85,630,50);
  text("X attuale = ",10,70);
  text(xUniciclo-xBase,110,70);
  text("Y attuale = ",10,90);
  text(-zUniciclo,110,90);
  text("x desiderata",10,110);
  text(xPunto-xBase,110,110);
  text("y desiderata",10,130);
  text(-zPunto,110,130);
  text("giunto selezionato = ",10,150);
  text(giunto+1,150,150);
  text("altezza camera",10,170);
  text(eyeY,120,170);
  text("thetaUniciclo = ",10,190);
  text(thetaUniciclo,120,190);
  text("thetaDes = ",10,210);
  text(thetaDes,120,210);
  } 


// funzione che permette il movimento dello scara
void muovi(){
  if (giunto == 0)
  {
    theta[giunto] += segno*.02;
  }
  if (giunto == 1)
  {
    if (segno*theta[giunto]-165*PI/180<0)
    {
      theta[giunto] += segno*.02;
    }
  }
  if (giunto == 2)
  {
    if (segno*theta[giunto]-d3y/2+d2y/2<0)
    {
      theta[giunto] += segno*1;
    }
  }
}
