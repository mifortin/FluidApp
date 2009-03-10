//
//	Field exports functionality for large matrices.  These matrices are
//		normally quite large, and are limited to powers of two.
//

#ifndef FIELD_H
#define FIELD_H

typedef struct field field;

field *fieldCreate(int m_width, int m_height, int m_components);

#endif
