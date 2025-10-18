//******** PRACTICA VISUALITZACI� GR�FICA INTERACTIVA (EE-UAB)
//******** Entorn b�sic VS2022 amb interf�cie MFC/GLFW i Status Bar
//******** Enric Marti (Setembre 2025)
// gouraud_shdrML.frag: Fragment Program en GLSL en versi� OpenGL 3.3 o 4.60 per a implementar:
//     a) Iluminaci� de Gouraud
//     b) Fonts de llum puntuals o direccionals
//     c) Fonts de llum restringides
//     d) Atenuaci� de fonts de llum
//     e) Modus d'assignaci� de textura MODULATE o DECAL
//     f) Calcul component a de transpar�ncia segons color sigui provinent de material o de VAO
//     g) Algorisme incremental iluminaci� Gouraud encapsulat en gouraudModel()

//#version 330 core
#version 460 core

// --- L16- Variables in
in vec4 VertexColor;	// Color del V�rtex
in vec2 VertexTexCoord;	// Coordenades textura del v�rtex

// --- L20- Variables uniform
uniform sampler2D texture0;	// Imatge textura
uniform bool textur;		// Booleana d�activaci� (TRUE) de textures o no (FALSE).
uniform bool modulate;		// Booleana d'activaci� de barreja color textura- color intensitat llum (TRUE) o nom�s color textura (FALSE)

// --- L25- Variables out
out vec4 FragColor;		// Color fragment (r,g,b,a)

void main ()
{
if (textur) {	// Intensitat amb textura
		vec4 colorT = texture(texture0,VertexTexCoord);
		// Textura modulada amb intensitat llum
	    	if (modulate) FragColor = colorT * VertexColor;
       			else FragColor=colorT; // textura sense modular intensitat llum
    	    }
    else { // Intensitat sense textura
           FragColor = VertexColor;   
         }
}
