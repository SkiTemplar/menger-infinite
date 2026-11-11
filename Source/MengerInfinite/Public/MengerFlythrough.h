#pragma once

#include "CoreMinimal.h"
#include "MengerNavigator.h"

/**
 * Recorrido de cámara para el vídeo (-MengerFlythrough), como función pura.
 *
 * La cámara baja de forma continua hacia F = (-1/3, -1/3, -1/3), la esquina del subcubo (-1, -1, -1)
 * que toca el hueco central. Alrededor de F hay 8 celdas del nivel 1: cuatro sólidas (la de la esquina
 * y las tres de arista) y cuatro huecas (el centro y las tres de cara). Cada celda sólida es una copia de
 * la esponja que toca F por una esquina, y la esquina de una esponja es un punto fijo de su subcubo de
 * esquina; por eso la geometría cercana a F es invariante al escalar por 3 con centro en F.
 *
 * La cámara se queda a Distance unidades de vista de F mientras la escala de vista crece linealmente con
 * el tiempo: zoom exponencial, velocidad percibida constante. Bajar Levels niveles enteros da la misma
 * imagen que el principio (bucle sin costura) siempre que la niebla tape lo que no es autosimilar: el
 * entorno autosimilar mide 3^(Scale-1) unidades de vista y la niebla satura a FogStart·3^FogLevels
 * (2·3^14 por defecto), así que hace falta Scale >= 16.
 *
 * Mirando a F en la dirección -(1, 1, 1), todo rayo con a lo sumo una componente positiva acaba en un
 * octante sólido. El borde de esa región está a 54,7° del eje y la diagonal del encuadre de 16:9 con el
 * FOV del proyecto llega a 45,9°: el balanceo tiene que quedarse por debajo de ~8° para no ver cielo.
 */
namespace Menger
{
	struct FFlythroughParams
	{
		/** Escala de vista del primer frame. */
		double StartScale = 20.0;
		/** Niveles que se bajan en un ciclo; entero para que el último frame repita el primero. */
		int32 Levels = 2;
		/** Distancia de la cámara a F en unidades de vista. */
		double Distance = 3.0;
		/** Balanceo de la cámara alrededor de F (grados): guiñada con un ciclo por bucle. */
		double YawAmplitude = 6.0;
		/** Cabeceo con dos ciclos por bucle (trayectoria en ocho). */
		double PitchAmplitude = 2.0;
	};

	/** Esquina F en el nivel Level: dígitos (-1, -1, -1), (1, 1, 1)... y Local = (1, 1, 1). */
	FFractalPosition MakeFlythroughTarget(int32 Level);

	/** Pose del recorrido en Phase (1 = un ciclo); periódica en Phase salvo por la escala, que sube Levels. */
	FCameraPose MakeFlythroughPose(const FFlythroughParams& Params, double Phase);
}
