Copier le dossier <em>Code_laser_beam_analyser</em> sur le bureau du <strong>raspberry Pi</strong>.
Le système necessite une <strong>caméra</strong>.

<ul>
<li>1ere méthode, interface classique :<br/>
Aller dans le dossier puis cliquer sur "LaserPI"</li>



<li>2e méthode, Terminal :<br/>
dans LXterminal :<br/>

Se déplacer dans le dossier : cd Desktop/Code_laser_beam_analyser

Demarrage du programme :  ./LaserPI

Compilation (Si LaserPI ne fonctionne pas ou si changement de code) :<br/>
g++ $(pkg-config --cflags --libs opencv) -lraspicam_cv -lraspicam  main.cpp cvplot.cpp pixel.cpp -o LaserPI -Wno-psabi
</li></ul>


<p align="center">
  <img src="../gallery/terminal.png" title="Terminal RaspberryPi">
</p>


