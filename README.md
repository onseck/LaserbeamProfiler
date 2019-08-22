# LaserbeamProfiler

<p align="center">
  <img src="gallery/logos.png" title="Logos">
</p>

<div style="display: inline-block">
<strong>Collaborateurs : </strong>
<ul>
  <li>Martial Geiser</li>
  <li>Frederic Truffer</li>
  <li>Alex Miollany</li>
  <li>Serge Amoos</li>
  <li>Christopher Tremblay</li>
</ul>
</div>

<div style="display: inline-block">
<strong>Liste du matériel</strong>
<ul>
  <li>Raspberry Pi, au moins model 3</li>
  <li>Caméra NoIR RaspberryPi Module V2</li>
  <li>Filtres optiques</li>
  <li>Ecran Raspberry Pi 7TD</li>
  <li>Alimentation 5V 2A</li>
</ul>
</div>



<br/><br/><br/>
<h3>Présentation</h3>

Un profileur laser est un dispositif coûteux qui est utilisé pour mesurer différentes caractéristiques d'un laser, notamment son diamètre et sa puissance (relative au filtre devant le capteur). Mesures utiles si on veut connaître la focalisation du laser par exemple.<br/> 
Le but de ce projet est de concevoir un dispositif simple composé d'un Raspberry Pi, d'une caméra et d'un écran (ici tactile mais pas nécessairement). Le tout dans un boîtier imprimé en 3D. La programmation de ce système est en C++ avec la librairie openCV, langage jusqu'ici le plus rapide pour le traitement d'image. 

<p align="center">
  <img src="gallery/Laser_profiler.jpg" width="350" title="Laser Beam Analyser">
</p>



Le système est composé de plusieurs filtres pour réduire l'intensité du laser arrivant dans la caméra. Le Raspberry prend alors une photo de la "tache", analyse cette image en déterminant les extremums. Le maximum est alors ciblé sur l'image grâce à un viseur.<br/> 
Il faut ensuite tracer les profils selon x et y de l'image, qui donnent une distribution normale et permettent de déterminer la taille du laser à 13.5 et 50 pourcent.<br/> 
Différents réglages sont ajoutés pour pouvoir modifier les paramètres de l'image comme la saturation ou les contrastes. Les réglages intéressants sont le temps d'exposition, le nombre d'échantillon et le gain de la caméra.<br/> 
L'utilisateur aura accès uniquement au nombre d'échantillon (qui lisse la Gaussienne) et au seuil de largeur (critères typiquement utilisés dans la mesure des lasers).<br/> 

<br/><br/><br/>
<h3>Interface</h3>

Sur l'illustration ci-dessous le mode normal du dispositif, l'utilisateur a accès à l'image du laser, aux deux profils, aux diamètres selon x et y, à la puissance et à quelques réglages basiques. <br/> 
C'est cette interface qui serait utilisée par un utilisateur voulant mesurer les caractéristiques d'un laser dans des conditions normales.
<p align="center">
  <img src="gallery/Interface_Laser_profiler_normal.png" width="350" title="Interface simple Laser Beam Analyser">
</p>


Ci-dessous les réglages du mode "expert" du dispositif, ici les réglages plus avancés de la caméra sont disponibles.
<p align="center">
  <img src="gallery/Interface_Laser_profiler_expert.png" width="350" title="Interface expert Laser Beam Analyser">
</p>



L'image du laser est disponible selon 3 modes : Normal, absolu et relatif
<ul>
  <li>relatif : c'est l'image modifiée comme précédemment mais le maximum du spectre est le maximum de l'image.</li>
  <li>absolu : c'est l'image avec les couleurs modifiées selon le spectre. On fixe le maximum du spectre à 255 et on aligne les couleurs </li>
  <li>normal : c'est l'image du laser sans traitement après le passage dans les filtres optiques.</li>
</ul>
<p align="center">
  <img src="gallery/diffaffichage.png" width="350" title="Différents affichages Laser Beam Analyser">
</p>


L'utilisateur peut exporter ses résultats en mettant le système en pause : 
<p align="center">
  <img src="gallery/Interface_Laser_profiler_pause.png" width="350" title="Interface pause/exporté Laser Beam Analyser">
</p>


Le système propose aussi la description du projet via le bouton "about".


<br/><br/><br/>
<h3>Normes pour le calcul du diamètre</h3>

<strong>Formules des premiers moments :</strong><br/>

<img src="http://latex.codecogs.com/svg.latex?\bar{x}(z)=\frac{\int_{-\infty}^{+\infty} \int_{-\infty}^{+\infty}E(x,y,z)x dx dy}{\int_{-\infty}^{+\infty} \int_{-\infty}^{+\infty}E(x,y,z) dx dy}" border="0"/>
<br/><br/>
<img src="http://latex.codecogs.com/svg.latex?\bar{y}(z)=\frac{\int_{-\infty}^{+\infty} \int_{-\infty}^{+\infty}E(x,y,z)y dx dy}{\int_{-\infty}^{+\infty} \int_{-\infty}^{+\infty}E(x,y,z) dx dy}" border="0"/>

<br/><br/>
<strong>Formules des seconds moments :</strong><br/>

<img src="http://latex.codecogs.com/svg.latex?\sigma_{x}^{2}(z)=\frac{\int_{-\infty}^{+\infty} \int_{-\infty}^{+\infty}E(x,y,z)(x-\bar{x})^{2} dx dy}{\int_{-\infty}^{+\infty} \int_{-\infty}^{+\infty}E(x,y,z) dx dy}" border="0"/>
<br/><br/>
<img src="http://latex.codecogs.com/svg.latex?\sigma_{y}^{2}(z)=\frac{\int_{-\infty}^{+\infty} \int_{-\infty}^{+\infty}E(x,y,z)(y-\bar{y})^{2} dx dy}{\int_{-\infty}^{+\infty} \int_{-\infty}^{+\infty}E(x,y,z) dx dy}" border="0"/>
<br/><br/>
<img src="http://latex.codecogs.com/svg.latex?\sigma_{xy}^{2}(z)=\frac{\int_{-\infty}^{+\infty} \int_{-\infty}^{+\infty}E(x,y,z)(x-\bar{x})(y-\bar{y}) dx dy}{\int_{-\infty}^{+\infty} \int_{-\infty}^{+\infty}E(x,y,z) dx dy}" border="0"/>

<br/><br/>
<strong>Formules des diamètres :</strong><br/>

<img src="http://latex.codecogs.com/svg.latex?d_{\sigma_x}(z)=2\sqrt{2}(\sigma_x^{2}+\sigma_y^{2}+2\left | \sigma_x_y^{2} \right |)^{\frac{1}{2}}" border="0"/>
<br/><br/>
<img src="http://latex.codecogs.com/svg.latex?d_{\sigma_y}(z)=2\sqrt{2}(\sigma_x^{2}+\sigma_y^{2}-2\left | \sigma_x_y^{2} \right |)^{\frac{1}{2}}" border="0"/>
<br/><br/>
<img src="http://latex.codecogs.com/svg.latex?d_{\sigma_y}(z)=2\sqrt{2}(\sigma_x^{2}+\sigma_y^{2})^{\frac{1}{2}}" border="0"/>
