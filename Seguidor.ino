// cONFIGURAR LA PISTA
bool pistaFondoNegro = false; // false = Línea Negra sobre Blanco | true = Línea Blanca sobre Negro

int velocidadRecta = 180;            // Velocidad máxima al ir al centro
int velocidadCorreccionSuave = 110;   // Velocidad de la llanta interna en desvíos leves
int velocidadCurva = 135;            // Velocidad de la llanta externa en curvas cerradas
int velocidadReversa = 75;           // Velocidad de la llanta interna en curvas cerradas (Giro de tanque)
unsigned long tiempoPerdidaTotal = 0;

// MEMORIA DE CURVA
unsigned long tiempoUltimaCurva = 0;
int ultimaDireccion = 0; // 0 = Recto, 1 = Izquierda, 2 = Derecha
int retrasoCurva = 80;   // Milisegundos de giro extra para centrarse

// BALANCE DE MOTORES (Trim)
float compensacionIzquierda = 0.83; 

// ==========================================
// DEFINICIÓN DE PINES
// ==========================================
// Motores
const int PWMA = 21; const int AIN1 = 19; const int AIN2 = 18;
const int PWMB = 5;  const int BIN1 = 17; const int BIN2 = 16;

// Arreglo de 5 sensores (De izquierda a derecha)
const int SENSOR_EXTREMA_IZQ = 22; 
const int SENSOR_CENTRO_IZQ  = 13;
const int SENSOR_CENTRO      = 14;
const int SENSOR_CENTRO_DER  = 27;
const int SENSOR_EXTREMA_DER = 23; 

// FUNCIONES DE AYUDA

void controlMotores(int velIzq, int velDer) {
  velIzq = constrain(velIzq, -255, 255);
  velDer = constrain(velDer, -255, 255);

  // Motor Izquierdo
  if (velIzq > 0) {
    digitalWrite(AIN1, HIGH);  
    digitalWrite(AIN2, LOW);
    analogWrite(PWMA, velIzq); 
  } else if (velIzq < 0) {
    digitalWrite(AIN1, LOW);   
    digitalWrite(AIN2, HIGH);
    analogWrite(PWMA, -velIzq); 
  } else {
    digitalWrite(AIN1, HIGH); // Freno Activo  
    digitalWrite(AIN2, HIGH);
    analogWrite(PWMA, 0); 
  }

  // Motor Derecho
  if (velDer > 0) {
    digitalWrite(BIN1, HIGH);  
    digitalWrite(BIN2, LOW);
    analogWrite(PWMB, velDer);
  } else if (velDer < 0) {
    digitalWrite(BIN1, LOW);   
    digitalWrite(BIN2, HIGH);
    analogWrite(PWMB, -velDer);
  } else {
    digitalWrite(BIN1, HIGH); // Freno Activo   
    digitalWrite(BIN2, HIGH);
    analogWrite(PWMB, 0); 
  }
}

// definicion de los pines y eso
void setup() {
  Serial.begin(115200);

  pinMode(PWMA, OUTPUT); pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(PWMB, OUTPUT); pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);

  controlMotores(0, 0); // que inicien freno

  // Configurar los 5 sensores
  pinMode(SENSOR_EXTREMA_IZQ, INPUT);
  pinMode(SENSOR_CENTRO_IZQ, INPUT);
  pinMode(SENSOR_CENTRO, INPUT);
  pinMode(SENSOR_CENTRO_DER, INPUT);
  pinMode(SENSOR_EXTREMA_DER, INPUT);
  delay(2000); 
}

void loop() {
  // 1. LEER LOS 5 SENSORES
  bool extIzq = pistaFondoNegro ? !digitalRead(SENSOR_EXTREMA_IZQ) : digitalRead(SENSOR_EXTREMA_IZQ);
  bool cenIzq = pistaFondoNegro ? !digitalRead(SENSOR_CENTRO_IZQ)  : digitalRead(SENSOR_CENTRO_IZQ);
  bool centro = pistaFondoNegro ? !digitalRead(SENSOR_CENTRO)      : digitalRead(SENSOR_CENTRO);
  bool cenDer = pistaFondoNegro ? !digitalRead(SENSOR_CENTRO_DER)  : digitalRead(SENSOR_CENTRO_DER);
  bool extDer = pistaFondoNegro ? !digitalRead(SENSOR_EXTREMA_DER) : digitalRead(SENSOR_EXTREMA_DER);

  // 2. TOMA DE DECISIONES
  if (extIzq || extDer || cenIzq || cenDer || centro) {
    // Si cualquier sensor ve la línea, actualizamos el tiempo para el retraso futuro
    tiempoUltimaCurva = millis();

    if (extIzq) {
      // EXTREMO IZQ: Giro de tanque agresivo (Curvas cerradas)
      controlMotores(-velocidadReversa, velocidadCurva); 
      ultimaDireccion = 1;         
    }
    else if (extDer) {
      // EXTREMO DER: Giro de tanque agresivo (Curvas cerradas)
      controlMotores(velocidadCurva * compensacionIzquierda, -velocidadReversa); 
      ultimaDireccion = 2;         
    }
    else if (cenIzq) {
      // MEDIO IZQ: Corrección suave y fluida para no perder impulso
      controlMotores(velocidadCorreccionSuave * compensacionIzquierda, velocidadRecta);
      ultimaDireccion = 1;         
    }
    else if (cenDer) {
      // MEDIO DER: Corrección suave y fluida para no perder impulso
      controlMotores(velocidadRecta * compensacionIzquierda, velocidadCorreccionSuave);
      ultimaDireccion = 2;         
    }
    else if (centro) {
      // CENTRO: Objetivo alcanzado, avanzar recto
      controlMotores(velocidadRecta * compensacionIzquierda, velocidadRecta);
      ultimaDireccion = 0; 
    }
  }
  else {
    // 3. LÓGICA DE RETRAZO Y BÚSQUEDA (Cuando todos ven blanco)
    
    // Mientras estemos dentro del tiempo de retraso, mantenemos el giro para alinear
    if (millis() - tiempoUltimaCurva < retrasoCurva) {
      if (ultimaDireccion == 1) {
        controlMotores(-velocidadReversa, velocidadCurva); // Sigue girando a la izq
      } 
      else if (ultimaDireccion == 2) {
        controlMotores(velocidadCurva * compensacionIzquierda, -velocidadReversa); // Sigue girando a la der
      }
      else {
        // Si iba recto y perdió la línea, sigue adelante buscando
        controlMotores(velocidadRecta * compensacionIzquierda, velocidadRecta);
      }
    } 
    else {
      // Si el retraso terminó y sigue en blanco, NO PARAR. 
      // Continuar la búsqueda en la última dirección para no perder la inercia.
      if (ultimaDireccion == 1) controlMotores(-velocidadReversa, velocidadCurva);
      else if (ultimaDireccion == 2) controlMotores(velocidadCurva * compensacionIzquierda, -velocidadReversa);
      else controlMotores(velocidadRecta * compensacionIzquierda, velocidadRecta);
    }
  }
}
