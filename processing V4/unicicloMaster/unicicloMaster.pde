/*breve introduzione: istruzioni stampate  a schermo, che comunque riporto di seguito :istruzioni : i abilita l'inseguimento, o lo disabilita,
r resetta la posizione dell'uniciclo ad un punto prefissato, s lo ferma instantaneamente, freccia su aumenta la velocità longitudinale, freccia giu la diminuisce,
freccia dx aumenta quella rotazionale, freccia sx la diminuisce, d aumenta la velocità con cui queste velocità sono incrementate, f diminuisce questa accelerazione,
q aumenta l'altezza dell'uniciclo, w la diminuisce,+ aumenta l'altezza della camera, - la diminuisce, z aumenta la velocità con cui i giunti dello SCARA si muovono, 
x la diminuisce,sono riportati i valori di queste variabili a schermo.
*/

// Coordinate attuali uniciclo
float thetaUniciclo = 0;
float xUniciclo=50;
float yUniciclo=-8+700/2+200/2;//200 era d0y, ma non posso referenziare prima di averlo dichiarato e lo volevo comunque avere per primo yUniciclo rispetto d0y
float zUniciclo=0;


float omegaR = 0; // velocità angolare ruota destra
float omegaL = 0; // velocità angolare ruota sinistra
float omegaRp,omegaLp; // accelerazioni angolari delle ruote

float kp = 1; // coefficiente accelerazione


float eyeY = 0;
float[] pT= {1,0,0,0};
float[] sT= {0,1,0,0};
float[] tT= {0,0,1,0};
float[] qT= {0,0,0,1};
float xBase;
float yBase;
int segno = 1;
int giunto = 0;

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
float[] theta = {0,0,0}; 

float xPunto=0;
float yPunto=0;
float zPunto=0;
float k = 0.5;
float v1 = 0;
float v2 =0;
float e_p =0;
float thetaDes=0;
float altezzaUniciclo = 10;
int inseguimento = 0;


 float x,y;
  
// Coordinate desiderate effettore immesse con click mouse (rispetto base robot)
  float xDes = xUniciclo;
  float yDes = -zUniciclo;
  
// lunghezze link robot  
  float L1 = d1x; // lunghezza primo link
  float L2 = d2x; // lunghezza secondo link  
  
  float argCos2 = 0;
  float C2 = 0;
  float S2 = 0;
  float theta1Des = 0;
  float theta2Des = 0;
  float kpc = .1; // la kp del controllo
  float precisione = .000001; // la differenza tra angolo finale e desiderato
  float gomito = 1;
  float zDes= 0;
  int g = 0;
  float t1p,t2p = 0;


// fine dichiarazioni variabili 


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
  // per evitare dei problemi come scritto in breve intro faccio in modo di avere sempre angoli tra -360° e + 360°
 theta[0]=theta[0]%(2*PI);
 theta[1]=theta[1]%(2*PI);
 background(255);
 directionalLight(128, 128, 128, -1, 0, 1);   
 lights();
 //Permette di ruotare la vista:
 camera((width/2.0), height/2 - eyeY, (height/2.0) / tan(PI*60.0 / 360.0), width/2.0, height/2.0, 0, 0, 1, 0);  



  pushMatrix();
  translate(width/2,height/2,0); // metto l'origine al centro della finestra

  if (keyPressed)
  {
    if (key=='i'){
      inseguimento = 1;
    }
    if (key == 'o'){
      inseguimento = 0;
    }
    if (key == 'r'){ //reset a posizione prestabilita
    xUniciclo = 50;
    zUniciclo = 0;
    thetaUniciclo = 0;
    }
    if (key == 's'){ // stop 
      v1 = 0;
      v2 = 0;
    }
    if ( keyCode == UP) // aumento velocità longitudinale
    {
      v1 += 5*kp;
    }
    if (keyCode == DOWN )  // diminuisco velocità longitudinale
    {
      v1 -= 5*kp;
    }
    if (keyCode == LEFT)  // aumento velocità angolare
    {
      v2 += .1*kp;
    }
    if (keyCode == RIGHT)  // diminuisco velocità angolare
    {
      v2 -= .1*kp;
    }
    if (key == 'd') // aumento coefficiente accelerazione
    {
      kp =   min(2,kp+0.1);
    }
    if (key == 'f') // diminuisco coefficiente accelerazione
    {
      kp = max(.01,kp-.01);
    }
    if(key == 'q' ){
      altezzaUniciclo = min(50,altezzaUniciclo+0.1);
    }
    if (key == 'w'){
      altezzaUniciclo = max(10,altezzaUniciclo-0.1);
    }
  }    
  popMatrix();
  
  // Cinematica uniciclo
  xUniciclo = xUniciclo + v1*cos(thetaUniciclo)*0.01;
  zUniciclo = zUniciclo + v1*sin(thetaUniciclo)*0.01;
  thetaUniciclo = thetaUniciclo + v2*0.01;
  
  // disegno uniciclo  
  robot(xUniciclo,zUniciclo,thetaUniciclo);
  
  // coordinate punto proiezione  SCARA
  xPunto =  xBase+d1x*cos(theta[0])+170*cos(theta[0]+theta[1]);
  yPunto = yBase;
  zPunto = -d1x*sin(theta[0])-170*sin(theta[0]+theta[1]);
  
     if (keyPressed)
  {
    // movimento camera
    if (key == '-')
    {
      eyeY -= 5;
    }
    if (key == '+')
    {
      eyeY += 5;
    }
    if(key == 'z'){
      kpc = min(kpc+0.005,1);
    }
    if(key == 'x'){
      kpc = max(0.0001,kpc-0.005);
    }
  }
  //pavimentazione 
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
 
 // muri di contenimento, l'idea era quella di avere un limite oltre il quale l'uniciclo non si potesse muovere, poi ho abbandonato l'idea pensando fosse limitante, 
 // ho lasciato i muri per estetica 
 pushMatrix();
 translate(-25+24*25,yBase+d0y/2-10,-500-26);
 
 fill(110,34,34);
 box(50*24,20,1);
 popMatrix();
 pushMatrix();
 translate(-25+24*25,yBase+d0y/2-10,+500-26);
 
 fill(110,34,34);
 box(50*24,20,1);
 popMatrix();
 pushMatrix();
 translate(-25,yBase+d0y/2-10,-25);
 
 fill(110,34,34);
 box(1,20,50*20);
 popMatrix();
 pushMatrix();
 translate(-25+50*24,yBase+d0y/2-10,-25);
 
 fill(110,34,34);
 box(1,20,50*20);
 popMatrix();
 //fine muri di contenimento
 
 
 // chiamata  a funzione per muovere SCARA
  if (inseguimento ==1){
    muoviScara();
  }
 
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
  
  
  // resa degli angoli tra un intervallo di -180° e +180°
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
  
  // scritte varie
  textSize(15);
  fill(0);
  text("istruzioni : i abilita l'inseguimento, o lo disabilita, r resetta la posizione dell'uniciclo ad un punto prefissato, s lo ferma instantaneamente, freccia su ",10,20); 
  text("aumenta la velocità longitudinale, freccia giu la diminuisce, freccia dx aumenta quella rotazionale, freccia sx la diminuisce, d aumenta la velocità con cui ",10,35);
  text("queste velocità sono incrementate, f diminuisce questa accelerazione, q aumenta l'altezza dell'uniciclo, w la diminuisce,+ aumenta l'altezza della camera, ",10,50);
  text("- la diminuisce, z aumenta la velocità con cui i giunti dello SCARA si muovono, x la diminuisce,sono riportati i valori di queste variabili a schermo.",10,65);
  text("inseguimento = ",10,85);
  text(inseguimento,120,85);
  text("v1 = ",10,100);
  text(v1,50,100);
  text("v2 = ",10,120);
  text(v2,50,120);
  text("k velocità = ",10,140);
  text(kp,90,140);
  text("altezza camera",10,160);
  text(eyeY,120,160);
  text("costante giunti SCARA = ",10,180);
  text(kpc,180,180);
  text("x uniciclo attuale = ",10,200);
  text(xUniciclo,180,200);
  text("y uniciclo attuale = ",10,220);
  text(-zUniciclo,180,220);
  text("x proiezione SCARA = ",10,240);
  text(xPunto,180,240);
  text("y proiezione SCARA = ",10,260);
  text(-zPunto,180,260);
  text("variabili di giunto,theta 1 = ",10,280);
  text(t1p,180,280);
  text("theta 2 = ",250,280);
  text(t2p,340,280);
  text("d3 = ",10,300);
  text(theta[2],50,300);
  
 
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
  } 

//funzione che permette il movimento dello SCARA
void muoviScara(){
 
  if (xUniciclo >480){ // ho notato che cosi l'inseguimento dell'uniciclo è migliore, aggiungendo questa quantità quando sulle x sto a più di 480
  xDes = xUniciclo-xBase+30;
  }else{
  xDes = xUniciclo-xBase-30; // idem a sopra solo che tolgo quel valore
  }
  if(zUniciclo <0){
  yDes = -zUniciclo-10;
  }else{
  yDes = -zUniciclo+10;
  }
  zDes = +yBase-d3y/2-altezzaUniciclo-d0y/2-40;
 // calcola coordinate giunto:
  argCos2 = (pow(xDes,2)+pow(yDes,2)-pow(L1,2)-pow(L2,2))/(2*L1*L2);
  if (abs(argCos2)<=1)
  {
    theta2Des = gomito*acos(argCos2);
    C2 = cos(theta2Des);
    S2 = sin(theta2Des);    
    theta1Des = atan2(-L2*S2*xDes+(L1+L2*C2)*yDes,(L1+L2*C2)*xDes+L2*S2*yDes)+g*2*PI;
    
   // come sopra evito fuoriuscita dagli intervalli di (+180°,-180°) per la variabile che andrò a scrivere a schermo che mi rappresenta theta 1 e 2
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
  
  // evito salti dello SCARA quando atan2 passa da 180 a -180 e viceversa
    if(abs(theta1Des+2*PI-theta[0])<abs(theta1Des-theta[0])){
      theta1Des = theta1Des+2*PI;
      g = g+1;
    }else{
      if(abs(theta1Des-2*PI-theta[1])<abs(theta1Des-theta[0])){
        theta1Des = theta1Des-2*PI;
        g = g-1;
      }
    }

  // controllo il grado di precisione raggiunto
    if (abs(theta[0]-theta1Des)>precisione)
    {
      theta[0] += kpc*(theta1Des-theta[0]);
    }
    if (abs(theta[1]-theta2Des)>precisione)
    {
      theta[1] += kpc*(theta2Des-theta[1]);
    }
    theta[2]=theta[2]+kpc*(zDes-theta[2]);
  }
  else
  { // se sto fuori dallo spazio di lavoro
    textSize(25);
    fill(255,0,0);
    text("posizione fuori dallo spazio di lavoro",250,190); 
  }
}
  
  // funzione che mi disegna il robot uniciclo
void robot(float x, float z, float theta)
{
// funzione che disegna uniciclo in (x,y) con orientamento theta  
  pushMatrix();
  translate(x,yBase+d0y/2-altezzaUniciclo/2-1,z);
  rotateY(-theta);
  fill(255,0,0);
  box(40,altezzaUniciclo,20); // il robot
  fill(255,255,0);
  translate(20,-altezzaUniciclo/2-2.5,0);
  box(20,5,5);
  popMatrix();
}
