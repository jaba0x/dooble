/*
** Copyright (c) 2008 - present, Alexis Megas.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from Dooble without specific prior written permission.
**
** DOOBLE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** DOOBLE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <QList>
#include <QtDebug>
#include <QtEndian>
#include <QtMath>
#include <climits>

#include "dooble_hmac.h"
#include "dooble_pbkdf2.h"

dooble_pbkdf2::dooble_pbkdf2
(const QByteArray &password,
 const QByteArray &salt,
 int iterations_count,
 int output_size)
{
  m_iterations_count = qAbs(iterations_count);
  m_output_size = dooble_hmac::preferred_output_size_in_bits() *
    qCeil(qAbs(output_size) / dooble_hmac::preferred_output_size_in_bits());
  m_password = password;
  m_salt = salt;
}

dooble_pbkdf2::~dooble_pbkdf2()
{
  if(!m_password.isEmpty())
    {
      QByteArray zeros(m_password.length(), 0);

      m_password.replace(0, m_password.length(), zeros);
    }

  m_password.clear();
}

QByteArray dooble_pbkdf2::pbkdf2(dooble_hmac_function *function) const
{
  if(function == 0 ||
     m_iterations_count == 0 ||
     m_output_size == 0 ||
     m_password.isEmpty() ||
     m_salt.isEmpty())
    return QByteArray();

  /*
  ** Partial implementation of https://en.wikipedia.org/wiki/PBKDF2.
  */

  QList<QByteArray> T;
  int iterations = m_output_size /
    dooble_hmac::preferred_output_size_in_bits(); /*
						  ** Only 512-bit PRFs are
						  ** expected.
						  */

  for(int i = 1; i <= iterations; i++)
    {
      QByteArray INT_32_BE_i(static_cast<int> (sizeof(int)), 0);
      QByteArray U;
      QByteArray Ua;

      qToBigEndian(i, INT_32_BE_i.data());
      U = Ua = function(m_password, QByteArray(m_salt).append(INT_32_BE_i));

      for(int j = 2; j <= m_iterations_count; j++)
	{
	  QByteArray Ub(function(m_password, Ua));

	  U = x_or(U, Ub);
	  Ua = Ub;
	}

      T.append(U);
    }

  QByteArray bytes;

  for(int i = 0; i < T.size(); i++)
    bytes.append(T.at(i));

  return bytes;
}

QByteArray dooble_pbkdf2::x_or(const QByteArray &a, const QByteArray &b) const
{
  QByteArray c(qMin(a.length(), b.length()), 0);

  for(int i = 0; i < c.length(); i++)
    c[i] = a.at(i) ^ b.at(i);

  return c;
}

void dooble_pbkdf2::interrupt(void)
{
}

void dooble_pbkdf2::test1(void)
{
  /*
  ** Please see https://stackoverflow.com/questions/15593184/pbkdf2-hmac-sha-512-test-vectors.
  */

  dooble_pbkdf2 pbkdf2("passwordPASSWORDpassword",
		       "saltSALTsaltSALTsaltSALTsaltSALTsalt",
		       4096,
		       512);

  qDebug() << pbkdf2.pbkdf2(&dooble_hmac::sha2_512hmac).toHex();
}
