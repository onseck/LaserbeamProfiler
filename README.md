# LaserbeamProfiler
<strong>Présentation</strong>
Un profileur laser est un dispositif coûteux qui est utilisé pour mesurer différentes caractéristiques d'un laser, notamment son diamètre et sa puissance (relative au filtre devant le capteur). Mesures utiles si on veut connaître la focale du laser par exemple. \\
Le but de ce projet est de concevoir un dispositif simple composé d'un Raspberry Pi, d'une caméra et d'un écran (ici tactile mais pas nécessairement). Le tout dans un boîtier imprimé en 3D. La programmation de ce système est en c++ avec la librairie openCV, langage jusqu'ici le plus rapide pour le traitement d'image. 

<p align="center">
  <img src="gallery/Laser_profiler.jpg" width="350" title="Laser Beam Analyser">
</p>


Le système est composé de plusieurs filtres pour réduire l'intensité du laser arrivant dans la caméra. Le Raspberry prend alors une photo de la "tache", analyse cette image en déterminant les extremums. Le maximum est alors ciblé sur l'image grâce à un viseur.\\
Il faut ensuite tracer les profils selon x et y de l'image, qui donnent une distribution normale et permettent de déterminer la taille du laser à 13.5 et 50 pourcent.\\
Différents réglages sont ajoutés pour pouvoir modifier les paramètres de l'image comme la saturation ou les contrastes. Les réglages intéressants sont le temps d'exposition, le nombre d'échantillon et le gain de la caméra.\\
L'utilisateur aura accès uniquement au nombre d'échantillon (qui lisse la Gaussienne) et au seuil de largeur (critères typiquement utilisés dans la mesure des lasers).\\
\smallbreak

\paragraph{Interface\\}

Sur l'illustration ci-dessous le mode normal du dispositif, l'utilisateur a accès à l'image du laser, aux deux profils, aux diamètres selon x et y, à la puissance et à quelques réglages basiques. \\
C'est cette interface qui serait utilisée par un utilisateur voulant mesurer les caractéristiques d'un laser dans des conditions normales.\\ 
\begin{figure}[H]
    \centering
    \includegraphics[scale=0.42]{images/Interface_Laser_profiler_normal.png}
    \caption{Interface simple Laser Beam Analyser}
    \label{fig:my_label}
\end{figure}
\smallbreak

Ci-dessous les réglages du mode "expert" du dispositif, ici les réglages plus avancés de la caméra sont disponibles.\\
\begin{figure}[H]
    \centering
    \includegraphics[scale=0.42]{images/Interface_Laser_profiler_expert.png}
    \caption{Interface pro Laser Beam Analyser}
    \label{fig:my_label}
\end{figure}
\smallbreak


L'image du laser est disponible selon 3 modes : Normal, absolu et relatif\\
\begin{itemize}
    \item relatif : c'est l'image modifiée comme précédemment mais le maximum du spectre est le maximum de l'image.
    \item absolu : c'est l'image avec les couleurs modifiées selon le spectre. On fixe le maximum du spectre à 255 et on aligne les couleurs ensuite.
    \item normal : c'est l'image du laser sans traitement après le passage dans les filtres optiques.
\end{itemize}
\begin{figure}[H]
    \centering
    \includegraphics[scale=1.3]{images/diffaffichage.png}
    \caption{Différents affichage de l'image du laser}
    \label{fig:my_label}
\end{figure}
\smallbreak



L'utilisateur peut exporter ses résultats en mettant le système en pause : 
\begin{figure}[H]
    \centering
    \includegraphics[scale=0.42]{images/Interface_Laser_profiler_pause.png}
    \caption{Interface exportée du Laser Beam Analyser}
    \label{fig:my_label}
\end{figure}
\smallbreak

Le système propose aussi la description du projet via le bouton "about".

\label{PreviousLastPage}
<p align="center">
  <img src="Laser_profiler.jpg" width="350" title="Laser Beam Analyser">
</p>
