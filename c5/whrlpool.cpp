// Whrlpool.cpp - modified by Kevin Springle from
// Paulo Barreto and Vincent Rijmen's public domain code, whirlpool.c.
// Any modifications are placed in the public domain

// This is the original introductory comment:

/**
 * The Whirlpool hashing function.
 *
 * <P>
 * <b>References</b>
 *
 * <P>
 * The Whirlpool algorithm was developed by
 * <a href="mailto:pbarreto@scopus.com.br">Paulo S. L. M. Barreto</a> and
 * <a href="mailto:vincent.rijmen@cryptomathic.com">Vincent Rijmen</a>.
 *
 * See
 *      P.S.L.M. Barreto, V. Rijmen,
 *      ``The Whirlpool hashing function,''
 *      NESSIE submission, 2000 (tweaked version, 2001),
 *      <https://www.cosic.esat.kuleuven.ac.be/nessie/workshop/submissions/whirlpool.zip>
 *
 * @author  Paulo S.L.M. Barreto
 * @author    Vincent Rijmen.
 *
 * @version 2.1 (2001.09.01)
 *
 * =============================================================================
 *
 * Differences from version 1.0:
 *
 * - Original S-box replaced by the tweaked, hardware-efficient version.
 *
 * =============================================================================
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ''AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "pch.h"

#ifdef WORD64_AVAILABLE

#include "whrlpool.h"
#include "misc.h"

NAMESPACE_BEGIN(CryptoPP)

void Whirlpool_TestInstantiations()
{
	Whirlpool x;
}

void Whirlpool::Init()
{
	m_digest[0] = m_digest[1] = m_digest[2] = m_digest[3] =
	m_digest[4] = m_digest[5] = m_digest[6] = m_digest[7] = 0;
}

void Whirlpool::TruncatedFinal(byte *hash, unsigned int size)
{
	ThrowIfInvalidTruncatedSize(size);

	PadLastBlock(32);
	CorrectEndianess(m_data, m_data, 32);

	m_data[m_data.size()-4] = 0;
	m_data[m_data.size()-3] = 0;
	m_data[m_data.size()-2] = GetBitCountHi();
	m_data[m_data.size()-1] = GetBitCountLo();

	Transform(m_digest, m_data);
	CorrectEndianess(m_digest, m_digest, DigestSize());
	memcpy(hash, m_digest, size);

	Restart();		// reinit for next use
}

/*
 * The number of rounds of the internal dedicated block cipher.
 */
#define R 10

/*
 * Though Whirlpool is endianness-neutral, the encryption tables are listed
 * in BIG-ENDIAN format, which is adopted throughout this implementation
 * (but little-endian notation would be equally suitable if consistently
 * employed).
 */

static const word64 C0[256] = {
	W64LIT(0x1818281878c0d878), W64LIT(0x23236523af0526af),
	W64LIT(0xc6c657c6f97eb8f9), W64LIT(0xe8e825e86f13fb6f),
	W64LIT(0x87879487a14ccba1), W64LIT(0xb8b8d5b862a91162),
	W64LIT(0x0101030105080905), W64LIT(0x4f4fd14f6e420d6e),
	W64LIT(0x36365a36eead9bee), W64LIT(0xa6a6f7a60459ff04),
	W64LIT(0xd2d26bd2bdde0cbd), W64LIT(0xf5f502f506fb0e06),
	W64LIT(0x79798b7980ef9680), W64LIT(0x6f6fb16fce5f30ce),
	W64LIT(0x9191ae91effc6def), W64LIT(0x5252f65207aaf807),
	W64LIT(0x6060a060fd2747fd), W64LIT(0xbcbcd9bc76893576),
	W64LIT(0x9b9bb09bcdac37cd), W64LIT(0x8e8e8f8e8c048a8c),
	W64LIT(0xa3a3f8a31571d215), W64LIT(0x0c0c140c3c606c3c),
	W64LIT(0x7b7b8d7b8aff848a), W64LIT(0x35355f35e1b580e1),
	W64LIT(0x1d1d271d69e8f569), W64LIT(0xe0e03de04753b347),
	W64LIT(0xd7d764d7acf621ac), W64LIT(0xc2c25bc2ed5e9ced),
	W64LIT(0x2e2e722e966d4396), W64LIT(0x4b4bdd4b7a62297a),
	W64LIT(0xfefe1ffe21a35d21), W64LIT(0x5757f9571682d516),
	W64LIT(0x15153f1541a8bd41), W64LIT(0x77779977b69fe8b6),
	W64LIT(0x37375937eba592eb), W64LIT(0xe5e532e5567b9e56),
	W64LIT(0x9f9fbc9fd98c13d9), W64LIT(0xf0f00df017d32317),
	W64LIT(0x4a4ade4a7f6a207f), W64LIT(0xdada73da959e4495),
	W64LIT(0x5858e85825faa225), W64LIT(0xc9c946c9ca06cfca),
	W64LIT(0x29297b298d557c8d), W64LIT(0x0a0a1e0a22505a22),
	W64LIT(0xb1b1ceb14fe1504f), W64LIT(0xa0a0fda01a69c91a),
	W64LIT(0x6b6bbd6bda7f14da), W64LIT(0x85859285ab5cd9ab),
	W64LIT(0xbdbddabd73813c73), W64LIT(0x5d5de75d34d28f34),
	W64LIT(0x1010301050809050), W64LIT(0xf4f401f403f30703),
	W64LIT(0xcbcb40cbc016ddc0), W64LIT(0x3e3e423ec6edd3c6),
	W64LIT(0x05050f0511282d11), W64LIT(0x6767a967e61f78e6),
	W64LIT(0xe4e431e453739753), W64LIT(0x27276927bb2502bb),
	W64LIT(0x4141c34158327358), W64LIT(0x8b8b808b9d2ca79d),
	W64LIT(0xa7a7f4a70151f601), W64LIT(0x7d7d877d94cfb294),
	W64LIT(0x9595a295fbdc49fb), W64LIT(0xd8d875d89f8e569f),
	W64LIT(0xfbfb10fb308b7030), W64LIT(0xeeee2fee7123cd71),
	W64LIT(0x7c7c847c91c7bb91), W64LIT(0x6666aa66e31771e3),
	W64LIT(0xdddd7add8ea67b8e), W64LIT(0x171739174bb8af4b),
	W64LIT(0x4747c94746024546), W64LIT(0x9e9ebf9edc841adc),
	W64LIT(0xcaca43cac51ed4c5), W64LIT(0x2d2d772d99755899),
	W64LIT(0xbfbfdcbf79912e79), W64LIT(0x070709071b383f1b),
	W64LIT(0xadadeaad2301ac23), W64LIT(0x5a5aee5a2feab02f),
	W64LIT(0x83839883b56cefb5), W64LIT(0x33335533ff85b6ff),
	W64LIT(0x6363a563f23f5cf2), W64LIT(0x020206020a10120a),
	W64LIT(0xaaaae3aa38399338), W64LIT(0x71719371a8afdea8),
	W64LIT(0xc8c845c8cf0ec6cf), W64LIT(0x19192b197dc8d17d),
	W64LIT(0x4949db4970723b70), W64LIT(0xd9d976d99a865f9a),
	W64LIT(0xf2f20bf21dc3311d), W64LIT(0xe3e338e3484ba848),
	W64LIT(0x5b5bed5b2ae2b92a), W64LIT(0x888885889234bc92),
	W64LIT(0x9a9ab39ac8a43ec8), W64LIT(0x26266a26be2d0bbe),
	W64LIT(0x32325632fa8dbffa), W64LIT(0xb0b0cdb04ae9594a),
	W64LIT(0xe9e926e96a1bf26a), W64LIT(0x0f0f110f33787733),
	W64LIT(0xd5d562d5a6e633a6), W64LIT(0x80809d80ba74f4ba),
	W64LIT(0xbebedfbe7c99277c), W64LIT(0xcdcd4acdde26ebde),
	W64LIT(0x34345c34e4bd89e4), W64LIT(0x4848d848757a3275),
	W64LIT(0xffff1cff24ab5424), W64LIT(0x7a7a8e7a8ff78d8f),
	W64LIT(0x9090ad90eaf464ea), W64LIT(0x5f5fe15f3ec29d3e),
	W64LIT(0x20206020a01d3da0), W64LIT(0x6868b868d5670fd5),
	W64LIT(0x1a1a2e1a72d0ca72), W64LIT(0xaeaeefae2c19b72c),
	W64LIT(0xb4b4c1b45ec97d5e), W64LIT(0x5454fc54199ace19),
	W64LIT(0x9393a893e5ec7fe5), W64LIT(0x22226622aa0d2faa),
	W64LIT(0x6464ac64e90763e9), W64LIT(0xf1f10ef112db2a12),
	W64LIT(0x73739573a2bfcca2), W64LIT(0x121236125a90825a),
	W64LIT(0x4040c0405d3a7a5d), W64LIT(0x0808180828404828),
	W64LIT(0xc3c358c3e85695e8), W64LIT(0xecec29ec7b33df7b),
	W64LIT(0xdbdb70db90964d90), W64LIT(0xa1a1fea11f61c01f),
	W64LIT(0x8d8d8a8d831c9183), W64LIT(0x3d3d473dc9f5c8c9),
	W64LIT(0x9797a497f1cc5bf1), W64LIT(0x0000000000000000),
	W64LIT(0xcfcf4ccfd436f9d4), W64LIT(0x2b2b7d2b87456e87),
	W64LIT(0x76769a76b397e1b3), W64LIT(0x82829b82b064e6b0),
	W64LIT(0xd6d667d6a9fe28a9), W64LIT(0x1b1b2d1b77d8c377),
	W64LIT(0xb5b5c2b55bc1745b), W64LIT(0xafafecaf2911be29),
	W64LIT(0x6a6abe6adf771ddf), W64LIT(0x5050f0500dbaea0d),
	W64LIT(0x4545cf454c12574c), W64LIT(0xf3f308f318cb3818),
	W64LIT(0x30305030f09dadf0), W64LIT(0xefef2cef742bc474),
	W64LIT(0x3f3f413fc3e5dac3), W64LIT(0x5555ff551c92c71c),
	W64LIT(0xa2a2fba21079db10), W64LIT(0xeaea23ea6503e965),
	W64LIT(0x6565af65ec0f6aec), W64LIT(0xbabad3ba68b90368),
	W64LIT(0x2f2f712f93654a93), W64LIT(0xc0c05dc0e74e8ee7),
	W64LIT(0xdede7fde81be6081), W64LIT(0x1c1c241c6ce0fc6c),
	W64LIT(0xfdfd1afd2ebb462e), W64LIT(0x4d4dd74d64521f64),
	W64LIT(0x9292ab92e0e476e0), W64LIT(0x75759f75bc8ffabc),
	W64LIT(0x06060a061e30361e), W64LIT(0x8a8a838a9824ae98),
	W64LIT(0xb2b2cbb240f94b40), W64LIT(0xe6e637e659638559),
	W64LIT(0x0e0e120e36707e36), W64LIT(0x1f1f211f63f8e763),
	W64LIT(0x6262a662f73755f7), W64LIT(0xd4d461d4a3ee3aa3),
	W64LIT(0xa8a8e5a832298132), W64LIT(0x9696a796f4c452f4),
	W64LIT(0xf9f916f93a9b623a), W64LIT(0xc5c552c5f666a3f6),
	W64LIT(0x25256f25b13510b1), W64LIT(0x5959eb5920f2ab20),
	W64LIT(0x84849184ae54d0ae), W64LIT(0x72729672a7b7c5a7),
	W64LIT(0x39394b39ddd5ecdd), W64LIT(0x4c4cd44c615a1661),
	W64LIT(0x5e5ee25e3bca943b), W64LIT(0x7878887885e79f85),
	W64LIT(0x38384838d8dde5d8), W64LIT(0x8c8c898c86149886),
	W64LIT(0xd1d16ed1b2c617b2), W64LIT(0xa5a5f2a50b41e40b),
	W64LIT(0xe2e23be24d43a14d), W64LIT(0x6161a361f82f4ef8),
	W64LIT(0xb3b3c8b345f14245), W64LIT(0x21216321a51534a5),
	W64LIT(0x9c9cb99cd69408d6), W64LIT(0x1e1e221e66f0ee66),
	W64LIT(0x4343c54352226152), W64LIT(0xc7c754c7fc76b1fc),
	W64LIT(0xfcfc19fc2bb34f2b), W64LIT(0x04040c0414202414),
	W64LIT(0x5151f35108b2e308), W64LIT(0x9999b699c7bc25c7),
	W64LIT(0x6d6db76dc44f22c4), W64LIT(0x0d0d170d39686539),
	W64LIT(0xfafa13fa35837935), W64LIT(0xdfdf7cdf84b66984),
	W64LIT(0x7e7e827e9bd7a99b), W64LIT(0x24246c24b43d19b4),
	W64LIT(0x3b3b4d3bd7c5fed7), W64LIT(0xababe0ab3d319a3d),
	W64LIT(0xcece4fced13ef0d1), W64LIT(0x1111331155889955),
	W64LIT(0x8f8f8c8f890c8389), W64LIT(0x4e4ed24e6b4a046b),
	W64LIT(0xb7b7c4b751d16651), W64LIT(0xebeb20eb600be060),
	W64LIT(0x3c3c443cccfdc1cc), W64LIT(0x81819e81bf7cfdbf),
	W64LIT(0x9494a194fed440fe), W64LIT(0xf7f704f70ceb1c0c),
	W64LIT(0xb9b9d6b967a11867), W64LIT(0x131335135f988b5f),
	W64LIT(0x2c2c742c9c7d519c), W64LIT(0xd3d368d3b8d605b8),
	W64LIT(0xe7e734e75c6b8c5c), W64LIT(0x6e6eb26ecb5739cb),
	W64LIT(0xc4c451c4f36eaaf3), W64LIT(0x030305030f181b0f),
	W64LIT(0x5656fa56138adc13), W64LIT(0x4444cc44491a5e49),
	W64LIT(0x7f7f817f9edfa09e), W64LIT(0xa9a9e6a937218837),
	W64LIT(0x2a2a7e2a824d6782), W64LIT(0xbbbbd0bb6db10a6d),
	W64LIT(0xc1c15ec1e24687e2), W64LIT(0x5353f55302a2f102),
	W64LIT(0xdcdc79dc8bae728b), W64LIT(0x0b0b1d0b27585327),
	W64LIT(0x9d9dba9dd39c01d3), W64LIT(0x6c6cb46cc1472bc1),
	W64LIT(0x31315331f595a4f5), W64LIT(0x74749c74b987f3b9),
	W64LIT(0xf6f607f609e31509), W64LIT(0x4646ca46430a4c43),
	W64LIT(0xacace9ac2609a526), W64LIT(0x89898689973cb597),
	W64LIT(0x14143c1444a0b444), W64LIT(0xe1e13ee1425bba42),
	W64LIT(0x16163a164eb0a64e), W64LIT(0x3a3a4e3ad2cdf7d2),
	W64LIT(0x6969bb69d06f06d0), W64LIT(0x09091b092d48412d),
	W64LIT(0x70709070ada7d7ad), W64LIT(0xb6b6c7b654d96f54),
	W64LIT(0xd0d06dd0b7ce1eb7), W64LIT(0xeded2aed7e3bd67e),
	W64LIT(0xcccc49ccdb2ee2db), W64LIT(0x4242c642572a6857),
	W64LIT(0x9898b598c2b42cc2), W64LIT(0xa4a4f1a40e49ed0e),
	W64LIT(0x28287828885d7588), W64LIT(0x5c5ce45c31da8631),
	W64LIT(0xf8f815f83f936b3f), W64LIT(0x86869786a444c2a4),
};

static const word64 C1[256] = {
	W64LIT(0x781818281878c0d8), W64LIT(0xaf23236523af0526),
	W64LIT(0xf9c6c657c6f97eb8), W64LIT(0x6fe8e825e86f13fb),
	W64LIT(0xa187879487a14ccb), W64LIT(0x62b8b8d5b862a911),
	W64LIT(0x0501010301050809), W64LIT(0x6e4f4fd14f6e420d),
	W64LIT(0xee36365a36eead9b), W64LIT(0x04a6a6f7a60459ff),
	W64LIT(0xbdd2d26bd2bdde0c), W64LIT(0x06f5f502f506fb0e),
	W64LIT(0x8079798b7980ef96), W64LIT(0xce6f6fb16fce5f30),
	W64LIT(0xef9191ae91effc6d), W64LIT(0x075252f65207aaf8),
	W64LIT(0xfd6060a060fd2747), W64LIT(0x76bcbcd9bc768935),
	W64LIT(0xcd9b9bb09bcdac37), W64LIT(0x8c8e8e8f8e8c048a),
	W64LIT(0x15a3a3f8a31571d2), W64LIT(0x3c0c0c140c3c606c),
	W64LIT(0x8a7b7b8d7b8aff84), W64LIT(0xe135355f35e1b580),
	W64LIT(0x691d1d271d69e8f5), W64LIT(0x47e0e03de04753b3),
	W64LIT(0xacd7d764d7acf621), W64LIT(0xedc2c25bc2ed5e9c),
	W64LIT(0x962e2e722e966d43), W64LIT(0x7a4b4bdd4b7a6229),
	W64LIT(0x21fefe1ffe21a35d), W64LIT(0x165757f9571682d5),
	W64LIT(0x4115153f1541a8bd), W64LIT(0xb677779977b69fe8),
	W64LIT(0xeb37375937eba592), W64LIT(0x56e5e532e5567b9e),
	W64LIT(0xd99f9fbc9fd98c13), W64LIT(0x17f0f00df017d323),
	W64LIT(0x7f4a4ade4a7f6a20), W64LIT(0x95dada73da959e44),
	W64LIT(0x255858e85825faa2), W64LIT(0xcac9c946c9ca06cf),
	W64LIT(0x8d29297b298d557c), W64LIT(0x220a0a1e0a22505a),
	W64LIT(0x4fb1b1ceb14fe150), W64LIT(0x1aa0a0fda01a69c9),
	W64LIT(0xda6b6bbd6bda7f14), W64LIT(0xab85859285ab5cd9),
	W64LIT(0x73bdbddabd73813c), W64LIT(0x345d5de75d34d28f),
	W64LIT(0x5010103010508090), W64LIT(0x03f4f401f403f307),
	W64LIT(0xc0cbcb40cbc016dd), W64LIT(0xc63e3e423ec6edd3),
	W64LIT(0x1105050f0511282d), W64LIT(0xe66767a967e61f78),
	W64LIT(0x53e4e431e4537397), W64LIT(0xbb27276927bb2502),
	W64LIT(0x584141c341583273), W64LIT(0x9d8b8b808b9d2ca7),
	W64LIT(0x01a7a7f4a70151f6), W64LIT(0x947d7d877d94cfb2),
	W64LIT(0xfb9595a295fbdc49), W64LIT(0x9fd8d875d89f8e56),
	W64LIT(0x30fbfb10fb308b70), W64LIT(0x71eeee2fee7123cd),
	W64LIT(0x917c7c847c91c7bb), W64LIT(0xe36666aa66e31771),
	W64LIT(0x8edddd7add8ea67b), W64LIT(0x4b171739174bb8af),
	W64LIT(0x464747c947460245), W64LIT(0xdc9e9ebf9edc841a),
	W64LIT(0xc5caca43cac51ed4), W64LIT(0x992d2d772d997558),
	W64LIT(0x79bfbfdcbf79912e), W64LIT(0x1b070709071b383f),
	W64LIT(0x23adadeaad2301ac), W64LIT(0x2f5a5aee5a2feab0),
	W64LIT(0xb583839883b56cef), W64LIT(0xff33335533ff85b6),
	W64LIT(0xf26363a563f23f5c), W64LIT(0x0a020206020a1012),
	W64LIT(0x38aaaae3aa383993), W64LIT(0xa871719371a8afde),
	W64LIT(0xcfc8c845c8cf0ec6), W64LIT(0x7d19192b197dc8d1),
	W64LIT(0x704949db4970723b), W64LIT(0x9ad9d976d99a865f),
	W64LIT(0x1df2f20bf21dc331), W64LIT(0x48e3e338e3484ba8),
	W64LIT(0x2a5b5bed5b2ae2b9), W64LIT(0x92888885889234bc),
	W64LIT(0xc89a9ab39ac8a43e), W64LIT(0xbe26266a26be2d0b),
	W64LIT(0xfa32325632fa8dbf), W64LIT(0x4ab0b0cdb04ae959),
	W64LIT(0x6ae9e926e96a1bf2), W64LIT(0x330f0f110f337877),
	W64LIT(0xa6d5d562d5a6e633), W64LIT(0xba80809d80ba74f4),
	W64LIT(0x7cbebedfbe7c9927), W64LIT(0xdecdcd4acdde26eb),
	W64LIT(0xe434345c34e4bd89), W64LIT(0x754848d848757a32),
	W64LIT(0x24ffff1cff24ab54), W64LIT(0x8f7a7a8e7a8ff78d),
	W64LIT(0xea9090ad90eaf464), W64LIT(0x3e5f5fe15f3ec29d),
	W64LIT(0xa020206020a01d3d), W64LIT(0xd56868b868d5670f),
	W64LIT(0x721a1a2e1a72d0ca), W64LIT(0x2caeaeefae2c19b7),
	W64LIT(0x5eb4b4c1b45ec97d), W64LIT(0x195454fc54199ace),
	W64LIT(0xe59393a893e5ec7f), W64LIT(0xaa22226622aa0d2f),
	W64LIT(0xe96464ac64e90763), W64LIT(0x12f1f10ef112db2a),
	W64LIT(0xa273739573a2bfcc), W64LIT(0x5a121236125a9082),
	W64LIT(0x5d4040c0405d3a7a), W64LIT(0x2808081808284048),
	W64LIT(0xe8c3c358c3e85695), W64LIT(0x7becec29ec7b33df),
	W64LIT(0x90dbdb70db90964d), W64LIT(0x1fa1a1fea11f61c0),
	W64LIT(0x838d8d8a8d831c91), W64LIT(0xc93d3d473dc9f5c8),
	W64LIT(0xf19797a497f1cc5b), W64LIT(0x0000000000000000),
	W64LIT(0xd4cfcf4ccfd436f9), W64LIT(0x872b2b7d2b87456e),
	W64LIT(0xb376769a76b397e1), W64LIT(0xb082829b82b064e6),
	W64LIT(0xa9d6d667d6a9fe28), W64LIT(0x771b1b2d1b77d8c3),
	W64LIT(0x5bb5b5c2b55bc174), W64LIT(0x29afafecaf2911be),
	W64LIT(0xdf6a6abe6adf771d), W64LIT(0x0d5050f0500dbaea),
	W64LIT(0x4c4545cf454c1257), W64LIT(0x18f3f308f318cb38),
	W64LIT(0xf030305030f09dad), W64LIT(0x74efef2cef742bc4),
	W64LIT(0xc33f3f413fc3e5da), W64LIT(0x1c5555ff551c92c7),
	W64LIT(0x10a2a2fba21079db), W64LIT(0x65eaea23ea6503e9),
	W64LIT(0xec6565af65ec0f6a), W64LIT(0x68babad3ba68b903),
	W64LIT(0x932f2f712f93654a), W64LIT(0xe7c0c05dc0e74e8e),
	W64LIT(0x81dede7fde81be60), W64LIT(0x6c1c1c241c6ce0fc),
	W64LIT(0x2efdfd1afd2ebb46), W64LIT(0x644d4dd74d64521f),
	W64LIT(0xe09292ab92e0e476), W64LIT(0xbc75759f75bc8ffa),
	W64LIT(0x1e06060a061e3036), W64LIT(0x988a8a838a9824ae),
	W64LIT(0x40b2b2cbb240f94b), W64LIT(0x59e6e637e6596385),
	W64LIT(0x360e0e120e36707e), W64LIT(0x631f1f211f63f8e7),
	W64LIT(0xf76262a662f73755), W64LIT(0xa3d4d461d4a3ee3a),
	W64LIT(0x32a8a8e5a8322981), W64LIT(0xf49696a796f4c452),
	W64LIT(0x3af9f916f93a9b62), W64LIT(0xf6c5c552c5f666a3),
	W64LIT(0xb125256f25b13510), W64LIT(0x205959eb5920f2ab),
	W64LIT(0xae84849184ae54d0), W64LIT(0xa772729672a7b7c5),
	W64LIT(0xdd39394b39ddd5ec), W64LIT(0x614c4cd44c615a16),
	W64LIT(0x3b5e5ee25e3bca94), W64LIT(0x857878887885e79f),
	W64LIT(0xd838384838d8dde5), W64LIT(0x868c8c898c861498),
	W64LIT(0xb2d1d16ed1b2c617), W64LIT(0x0ba5a5f2a50b41e4),
	W64LIT(0x4de2e23be24d43a1), W64LIT(0xf86161a361f82f4e),
	W64LIT(0x45b3b3c8b345f142), W64LIT(0xa521216321a51534),
	W64LIT(0xd69c9cb99cd69408), W64LIT(0x661e1e221e66f0ee),
	W64LIT(0x524343c543522261), W64LIT(0xfcc7c754c7fc76b1),
	W64LIT(0x2bfcfc19fc2bb34f), W64LIT(0x1404040c04142024),
	W64LIT(0x085151f35108b2e3), W64LIT(0xc79999b699c7bc25),
	W64LIT(0xc46d6db76dc44f22), W64LIT(0x390d0d170d396865),
	W64LIT(0x35fafa13fa358379), W64LIT(0x84dfdf7cdf84b669),
	W64LIT(0x9b7e7e827e9bd7a9), W64LIT(0xb424246c24b43d19),
	W64LIT(0xd73b3b4d3bd7c5fe), W64LIT(0x3dababe0ab3d319a),
	W64LIT(0xd1cece4fced13ef0), W64LIT(0x5511113311558899),
	W64LIT(0x898f8f8c8f890c83), W64LIT(0x6b4e4ed24e6b4a04),
	W64LIT(0x51b7b7c4b751d166), W64LIT(0x60ebeb20eb600be0),
	W64LIT(0xcc3c3c443cccfdc1), W64LIT(0xbf81819e81bf7cfd),
	W64LIT(0xfe9494a194fed440), W64LIT(0x0cf7f704f70ceb1c),
	W64LIT(0x67b9b9d6b967a118), W64LIT(0x5f131335135f988b),
	W64LIT(0x9c2c2c742c9c7d51), W64LIT(0xb8d3d368d3b8d605),
	W64LIT(0x5ce7e734e75c6b8c), W64LIT(0xcb6e6eb26ecb5739),
	W64LIT(0xf3c4c451c4f36eaa), W64LIT(0x0f030305030f181b),
	W64LIT(0x135656fa56138adc), W64LIT(0x494444cc44491a5e),
	W64LIT(0x9e7f7f817f9edfa0), W64LIT(0x37a9a9e6a9372188),
	W64LIT(0x822a2a7e2a824d67), W64LIT(0x6dbbbbd0bb6db10a),
	W64LIT(0xe2c1c15ec1e24687), W64LIT(0x025353f55302a2f1),
	W64LIT(0x8bdcdc79dc8bae72), W64LIT(0x270b0b1d0b275853),
	W64LIT(0xd39d9dba9dd39c01), W64LIT(0xc16c6cb46cc1472b),
	W64LIT(0xf531315331f595a4), W64LIT(0xb974749c74b987f3),
	W64LIT(0x09f6f607f609e315), W64LIT(0x434646ca46430a4c),
	W64LIT(0x26acace9ac2609a5), W64LIT(0x9789898689973cb5),
	W64LIT(0x4414143c1444a0b4), W64LIT(0x42e1e13ee1425bba),
	W64LIT(0x4e16163a164eb0a6), W64LIT(0xd23a3a4e3ad2cdf7),
	W64LIT(0xd06969bb69d06f06), W64LIT(0x2d09091b092d4841),
	W64LIT(0xad70709070ada7d7), W64LIT(0x54b6b6c7b654d96f),
	W64LIT(0xb7d0d06dd0b7ce1e), W64LIT(0x7eeded2aed7e3bd6),
	W64LIT(0xdbcccc49ccdb2ee2), W64LIT(0x574242c642572a68),
	W64LIT(0xc29898b598c2b42c), W64LIT(0x0ea4a4f1a40e49ed),
	W64LIT(0x8828287828885d75), W64LIT(0x315c5ce45c31da86),
	W64LIT(0x3ff8f815f83f936b), W64LIT(0xa486869786a444c2),
};

static const word64 C2[256] = {
	W64LIT(0xd8781818281878c0), W64LIT(0x26af23236523af05),
	W64LIT(0xb8f9c6c657c6f97e), W64LIT(0xfb6fe8e825e86f13),
	W64LIT(0xcba187879487a14c), W64LIT(0x1162b8b8d5b862a9),
	W64LIT(0x0905010103010508), W64LIT(0x0d6e4f4fd14f6e42),
	W64LIT(0x9bee36365a36eead), W64LIT(0xff04a6a6f7a60459),
	W64LIT(0x0cbdd2d26bd2bdde), W64LIT(0x0e06f5f502f506fb),
	W64LIT(0x968079798b7980ef), W64LIT(0x30ce6f6fb16fce5f),
	W64LIT(0x6def9191ae91effc), W64LIT(0xf8075252f65207aa),
	W64LIT(0x47fd6060a060fd27), W64LIT(0x3576bcbcd9bc7689),
	W64LIT(0x37cd9b9bb09bcdac), W64LIT(0x8a8c8e8e8f8e8c04),
	W64LIT(0xd215a3a3f8a31571), W64LIT(0x6c3c0c0c140c3c60),
	W64LIT(0x848a7b7b8d7b8aff), W64LIT(0x80e135355f35e1b5),
	W64LIT(0xf5691d1d271d69e8), W64LIT(0xb347e0e03de04753),
	W64LIT(0x21acd7d764d7acf6), W64LIT(0x9cedc2c25bc2ed5e),
	W64LIT(0x43962e2e722e966d), W64LIT(0x297a4b4bdd4b7a62),
	W64LIT(0x5d21fefe1ffe21a3), W64LIT(0xd5165757f9571682),
	W64LIT(0xbd4115153f1541a8), W64LIT(0xe8b677779977b69f),
	W64LIT(0x92eb37375937eba5), W64LIT(0x9e56e5e532e5567b),
	W64LIT(0x13d99f9fbc9fd98c), W64LIT(0x2317f0f00df017d3),
	W64LIT(0x207f4a4ade4a7f6a), W64LIT(0x4495dada73da959e),
	W64LIT(0xa2255858e85825fa), W64LIT(0xcfcac9c946c9ca06),
	W64LIT(0x7c8d29297b298d55), W64LIT(0x5a220a0a1e0a2250),
	W64LIT(0x504fb1b1ceb14fe1), W64LIT(0xc91aa0a0fda01a69),
	W64LIT(0x14da6b6bbd6bda7f), W64LIT(0xd9ab85859285ab5c),
	W64LIT(0x3c73bdbddabd7381), W64LIT(0x8f345d5de75d34d2),
	W64LIT(0x9050101030105080), W64LIT(0x0703f4f401f403f3),
	W64LIT(0xddc0cbcb40cbc016), W64LIT(0xd3c63e3e423ec6ed),
	W64LIT(0x2d1105050f051128), W64LIT(0x78e66767a967e61f),
	W64LIT(0x9753e4e431e45373), W64LIT(0x02bb27276927bb25),
	W64LIT(0x73584141c3415832), W64LIT(0xa79d8b8b808b9d2c),
	W64LIT(0xf601a7a7f4a70151), W64LIT(0xb2947d7d877d94cf),
	W64LIT(0x49fb9595a295fbdc), W64LIT(0x569fd8d875d89f8e),
	W64LIT(0x7030fbfb10fb308b), W64LIT(0xcd71eeee2fee7123),
	W64LIT(0xbb917c7c847c91c7), W64LIT(0x71e36666aa66e317),
	W64LIT(0x7b8edddd7add8ea6), W64LIT(0xaf4b171739174bb8),
	W64LIT(0x45464747c9474602), W64LIT(0x1adc9e9ebf9edc84),
	W64LIT(0xd4c5caca43cac51e), W64LIT(0x58992d2d772d9975),
	W64LIT(0x2e79bfbfdcbf7991), W64LIT(0x3f1b070709071b38),
	W64LIT(0xac23adadeaad2301), W64LIT(0xb02f5a5aee5a2fea),
	W64LIT(0xefb583839883b56c), W64LIT(0xb6ff33335533ff85),
	W64LIT(0x5cf26363a563f23f), W64LIT(0x120a020206020a10),
	W64LIT(0x9338aaaae3aa3839), W64LIT(0xdea871719371a8af),
	W64LIT(0xc6cfc8c845c8cf0e), W64LIT(0xd17d19192b197dc8),
	W64LIT(0x3b704949db497072), W64LIT(0x5f9ad9d976d99a86),
	W64LIT(0x311df2f20bf21dc3), W64LIT(0xa848e3e338e3484b),
	W64LIT(0xb92a5b5bed5b2ae2), W64LIT(0xbc92888885889234),
	W64LIT(0x3ec89a9ab39ac8a4), W64LIT(0x0bbe26266a26be2d),
	W64LIT(0xbffa32325632fa8d), W64LIT(0x594ab0b0cdb04ae9),
	W64LIT(0xf26ae9e926e96a1b), W64LIT(0x77330f0f110f3378),
	W64LIT(0x33a6d5d562d5a6e6), W64LIT(0xf4ba80809d80ba74),
	W64LIT(0x277cbebedfbe7c99), W64LIT(0xebdecdcd4acdde26),
	W64LIT(0x89e434345c34e4bd), W64LIT(0x32754848d848757a),
	W64LIT(0x5424ffff1cff24ab), W64LIT(0x8d8f7a7a8e7a8ff7),
	W64LIT(0x64ea9090ad90eaf4), W64LIT(0x9d3e5f5fe15f3ec2),
	W64LIT(0x3da020206020a01d), W64LIT(0x0fd56868b868d567),
	W64LIT(0xca721a1a2e1a72d0), W64LIT(0xb72caeaeefae2c19),
	W64LIT(0x7d5eb4b4c1b45ec9), W64LIT(0xce195454fc54199a),
	W64LIT(0x7fe59393a893e5ec), W64LIT(0x2faa22226622aa0d),
	W64LIT(0x63e96464ac64e907), W64LIT(0x2a12f1f10ef112db),
	W64LIT(0xcca273739573a2bf), W64LIT(0x825a121236125a90),
	W64LIT(0x7a5d4040c0405d3a), W64LIT(0x4828080818082840),
	W64LIT(0x95e8c3c358c3e856), W64LIT(0xdf7becec29ec7b33),
	W64LIT(0x4d90dbdb70db9096), W64LIT(0xc01fa1a1fea11f61),
	W64LIT(0x91838d8d8a8d831c), W64LIT(0xc8c93d3d473dc9f5),
	W64LIT(0x5bf19797a497f1cc), W64LIT(0x0000000000000000),
	W64LIT(0xf9d4cfcf4ccfd436), W64LIT(0x6e872b2b7d2b8745),
	W64LIT(0xe1b376769a76b397), W64LIT(0xe6b082829b82b064),
	W64LIT(0x28a9d6d667d6a9fe), W64LIT(0xc3771b1b2d1b77d8),
	W64LIT(0x745bb5b5c2b55bc1), W64LIT(0xbe29afafecaf2911),
	W64LIT(0x1ddf6a6abe6adf77), W64LIT(0xea0d5050f0500dba),
	W64LIT(0x574c4545cf454c12), W64LIT(0x3818f3f308f318cb),
	W64LIT(0xadf030305030f09d), W64LIT(0xc474efef2cef742b),
	W64LIT(0xdac33f3f413fc3e5), W64LIT(0xc71c5555ff551c92),
	W64LIT(0xdb10a2a2fba21079), W64LIT(0xe965eaea23ea6503),
	W64LIT(0x6aec6565af65ec0f), W64LIT(0x0368babad3ba68b9),
	W64LIT(0x4a932f2f712f9365), W64LIT(0x8ee7c0c05dc0e74e),
	W64LIT(0x6081dede7fde81be), W64LIT(0xfc6c1c1c241c6ce0),
	W64LIT(0x462efdfd1afd2ebb), W64LIT(0x1f644d4dd74d6452),
	W64LIT(0x76e09292ab92e0e4), W64LIT(0xfabc75759f75bc8f),
	W64LIT(0x361e06060a061e30), W64LIT(0xae988a8a838a9824),
	W64LIT(0x4b40b2b2cbb240f9), W64LIT(0x8559e6e637e65963),
	W64LIT(0x7e360e0e120e3670), W64LIT(0xe7631f1f211f63f8),
	W64LIT(0x55f76262a662f737), W64LIT(0x3aa3d4d461d4a3ee),
	W64LIT(0x8132a8a8e5a83229), W64LIT(0x52f49696a796f4c4),
	W64LIT(0x623af9f916f93a9b), W64LIT(0xa3f6c5c552c5f666),
	W64LIT(0x10b125256f25b135), W64LIT(0xab205959eb5920f2),
	W64LIT(0xd0ae84849184ae54), W64LIT(0xc5a772729672a7b7),
	W64LIT(0xecdd39394b39ddd5), W64LIT(0x16614c4cd44c615a),
	W64LIT(0x943b5e5ee25e3bca), W64LIT(0x9f857878887885e7),
	W64LIT(0xe5d838384838d8dd), W64LIT(0x98868c8c898c8614),
	W64LIT(0x17b2d1d16ed1b2c6), W64LIT(0xe40ba5a5f2a50b41),
	W64LIT(0xa14de2e23be24d43), W64LIT(0x4ef86161a361f82f),
	W64LIT(0x4245b3b3c8b345f1), W64LIT(0x34a521216321a515),
	W64LIT(0x08d69c9cb99cd694), W64LIT(0xee661e1e221e66f0),
	W64LIT(0x61524343c5435222), W64LIT(0xb1fcc7c754c7fc76),
	W64LIT(0x4f2bfcfc19fc2bb3), W64LIT(0x241404040c041420),
	W64LIT(0xe3085151f35108b2), W64LIT(0x25c79999b699c7bc),
	W64LIT(0x22c46d6db76dc44f), W64LIT(0x65390d0d170d3968),
	W64LIT(0x7935fafa13fa3583), W64LIT(0x6984dfdf7cdf84b6),
	W64LIT(0xa99b7e7e827e9bd7), W64LIT(0x19b424246c24b43d),
	W64LIT(0xfed73b3b4d3bd7c5), W64LIT(0x9a3dababe0ab3d31),
	W64LIT(0xf0d1cece4fced13e), W64LIT(0x9955111133115588),
	W64LIT(0x83898f8f8c8f890c), W64LIT(0x046b4e4ed24e6b4a),
	W64LIT(0x6651b7b7c4b751d1), W64LIT(0xe060ebeb20eb600b),
	W64LIT(0xc1cc3c3c443cccfd), W64LIT(0xfdbf81819e81bf7c),
	W64LIT(0x40fe9494a194fed4), W64LIT(0x1c0cf7f704f70ceb),
	W64LIT(0x1867b9b9d6b967a1), W64LIT(0x8b5f131335135f98),
	W64LIT(0x519c2c2c742c9c7d), W64LIT(0x05b8d3d368d3b8d6),
	W64LIT(0x8c5ce7e734e75c6b), W64LIT(0x39cb6e6eb26ecb57),
	W64LIT(0xaaf3c4c451c4f36e), W64LIT(0x1b0f030305030f18),
	W64LIT(0xdc135656fa56138a), W64LIT(0x5e494444cc44491a),
	W64LIT(0xa09e7f7f817f9edf), W64LIT(0x8837a9a9e6a93721),
	W64LIT(0x67822a2a7e2a824d), W64LIT(0x0a6dbbbbd0bb6db1),
	W64LIT(0x87e2c1c15ec1e246), W64LIT(0xf1025353f55302a2),
	W64LIT(0x728bdcdc79dc8bae), W64LIT(0x53270b0b1d0b2758),
	W64LIT(0x01d39d9dba9dd39c), W64LIT(0x2bc16c6cb46cc147),
	W64LIT(0xa4f531315331f595), W64LIT(0xf3b974749c74b987),
	W64LIT(0x1509f6f607f609e3), W64LIT(0x4c434646ca46430a),
	W64LIT(0xa526acace9ac2609), W64LIT(0xb59789898689973c),
	W64LIT(0xb44414143c1444a0), W64LIT(0xba42e1e13ee1425b),
	W64LIT(0xa64e16163a164eb0), W64LIT(0xf7d23a3a4e3ad2cd),
	W64LIT(0x06d06969bb69d06f), W64LIT(0x412d09091b092d48),
	W64LIT(0xd7ad70709070ada7), W64LIT(0x6f54b6b6c7b654d9),
	W64LIT(0x1eb7d0d06dd0b7ce), W64LIT(0xd67eeded2aed7e3b),
	W64LIT(0xe2dbcccc49ccdb2e), W64LIT(0x68574242c642572a),
	W64LIT(0x2cc29898b598c2b4), W64LIT(0xed0ea4a4f1a40e49),
	W64LIT(0x758828287828885d), W64LIT(0x86315c5ce45c31da),
	W64LIT(0x6b3ff8f815f83f93), W64LIT(0xc2a486869786a444),
};

static const word64 C3[256] = {
	W64LIT(0xc0d8781818281878), W64LIT(0x0526af23236523af),
	W64LIT(0x7eb8f9c6c657c6f9), W64LIT(0x13fb6fe8e825e86f),
	W64LIT(0x4ccba187879487a1), W64LIT(0xa91162b8b8d5b862),
	W64LIT(0x0809050101030105), W64LIT(0x420d6e4f4fd14f6e),
	W64LIT(0xad9bee36365a36ee), W64LIT(0x59ff04a6a6f7a604),
	W64LIT(0xde0cbdd2d26bd2bd), W64LIT(0xfb0e06f5f502f506),
	W64LIT(0xef968079798b7980), W64LIT(0x5f30ce6f6fb16fce),
	W64LIT(0xfc6def9191ae91ef), W64LIT(0xaaf8075252f65207),
	W64LIT(0x2747fd6060a060fd), W64LIT(0x893576bcbcd9bc76),
	W64LIT(0xac37cd9b9bb09bcd), W64LIT(0x048a8c8e8e8f8e8c),
	W64LIT(0x71d215a3a3f8a315), W64LIT(0x606c3c0c0c140c3c),
	W64LIT(0xff848a7b7b8d7b8a), W64LIT(0xb580e135355f35e1),
	W64LIT(0xe8f5691d1d271d69), W64LIT(0x53b347e0e03de047),
	W64LIT(0xf621acd7d764d7ac), W64LIT(0x5e9cedc2c25bc2ed),
	W64LIT(0x6d43962e2e722e96), W64LIT(0x62297a4b4bdd4b7a),
	W64LIT(0xa35d21fefe1ffe21), W64LIT(0x82d5165757f95716),
	W64LIT(0xa8bd4115153f1541), W64LIT(0x9fe8b677779977b6),
	W64LIT(0xa592eb37375937eb), W64LIT(0x7b9e56e5e532e556),
	W64LIT(0x8c13d99f9fbc9fd9), W64LIT(0xd32317f0f00df017),
	W64LIT(0x6a207f4a4ade4a7f), W64LIT(0x9e4495dada73da95),
	W64LIT(0xfaa2255858e85825), W64LIT(0x06cfcac9c946c9ca),
	W64LIT(0x557c8d29297b298d), W64LIT(0x505a220a0a1e0a22),
	W64LIT(0xe1504fb1b1ceb14f), W64LIT(0x69c91aa0a0fda01a),
	W64LIT(0x7f14da6b6bbd6bda), W64LIT(0x5cd9ab85859285ab),
	W64LIT(0x813c73bdbddabd73), W64LIT(0xd28f345d5de75d34),
	W64LIT(0x8090501010301050), W64LIT(0xf30703f4f401f403),
	W64LIT(0x16ddc0cbcb40cbc0), W64LIT(0xedd3c63e3e423ec6),
	W64LIT(0x282d1105050f0511), W64LIT(0x1f78e66767a967e6),
	W64LIT(0x739753e4e431e453), W64LIT(0x2502bb27276927bb),
	W64LIT(0x3273584141c34158), W64LIT(0x2ca79d8b8b808b9d),
	W64LIT(0x51f601a7a7f4a701), W64LIT(0xcfb2947d7d877d94),
	W64LIT(0xdc49fb9595a295fb), W64LIT(0x8e569fd8d875d89f),
	W64LIT(0x8b7030fbfb10fb30), W64LIT(0x23cd71eeee2fee71),
	W64LIT(0xc7bb917c7c847c91), W64LIT(0x1771e36666aa66e3),
	W64LIT(0xa67b8edddd7add8e), W64LIT(0xb8af4b171739174b),
	W64LIT(0x0245464747c94746), W64LIT(0x841adc9e9ebf9edc),
	W64LIT(0x1ed4c5caca43cac5), W64LIT(0x7558992d2d772d99),
	W64LIT(0x912e79bfbfdcbf79), W64LIT(0x383f1b070709071b),
	W64LIT(0x01ac23adadeaad23), W64LIT(0xeab02f5a5aee5a2f),
	W64LIT(0x6cefb583839883b5), W64LIT(0x85b6ff33335533ff),
	W64LIT(0x3f5cf26363a563f2), W64LIT(0x10120a020206020a),
	W64LIT(0x399338aaaae3aa38), W64LIT(0xafdea871719371a8),
	W64LIT(0x0ec6cfc8c845c8cf), W64LIT(0xc8d17d19192b197d),
	W64LIT(0x723b704949db4970), W64LIT(0x865f9ad9d976d99a),
	W64LIT(0xc3311df2f20bf21d), W64LIT(0x4ba848e3e338e348),
	W64LIT(0xe2b92a5b5bed5b2a), W64LIT(0x34bc928888858892),
	W64LIT(0xa43ec89a9ab39ac8), W64LIT(0x2d0bbe26266a26be),
	W64LIT(0x8dbffa32325632fa), W64LIT(0xe9594ab0b0cdb04a),
	W64LIT(0x1bf26ae9e926e96a), W64LIT(0x7877330f0f110f33),
	W64LIT(0xe633a6d5d562d5a6), W64LIT(0x74f4ba80809d80ba),
	W64LIT(0x99277cbebedfbe7c), W64LIT(0x26ebdecdcd4acdde),
	W64LIT(0xbd89e434345c34e4), W64LIT(0x7a32754848d84875),
	W64LIT(0xab5424ffff1cff24), W64LIT(0xf78d8f7a7a8e7a8f),
	W64LIT(0xf464ea9090ad90ea), W64LIT(0xc29d3e5f5fe15f3e),
	W64LIT(0x1d3da020206020a0), W64LIT(0x670fd56868b868d5),
	W64LIT(0xd0ca721a1a2e1a72), W64LIT(0x19b72caeaeefae2c),
	W64LIT(0xc97d5eb4b4c1b45e), W64LIT(0x9ace195454fc5419),
	W64LIT(0xec7fe59393a893e5), W64LIT(0x0d2faa22226622aa),
	W64LIT(0x0763e96464ac64e9), W64LIT(0xdb2a12f1f10ef112),
	W64LIT(0xbfcca273739573a2), W64LIT(0x90825a121236125a),
	W64LIT(0x3a7a5d4040c0405d), W64LIT(0x4048280808180828),
	W64LIT(0x5695e8c3c358c3e8), W64LIT(0x33df7becec29ec7b),
	W64LIT(0x964d90dbdb70db90), W64LIT(0x61c01fa1a1fea11f),
	W64LIT(0x1c91838d8d8a8d83), W64LIT(0xf5c8c93d3d473dc9),
	W64LIT(0xcc5bf19797a497f1), W64LIT(0x0000000000000000),
	W64LIT(0x36f9d4cfcf4ccfd4), W64LIT(0x456e872b2b7d2b87),
	W64LIT(0x97e1b376769a76b3), W64LIT(0x64e6b082829b82b0),
	W64LIT(0xfe28a9d6d667d6a9), W64LIT(0xd8c3771b1b2d1b77),
	W64LIT(0xc1745bb5b5c2b55b), W64LIT(0x11be29afafecaf29),
	W64LIT(0x771ddf6a6abe6adf), W64LIT(0xbaea0d5050f0500d),
	W64LIT(0x12574c4545cf454c), W64LIT(0xcb3818f3f308f318),
	W64LIT(0x9dadf030305030f0), W64LIT(0x2bc474efef2cef74),
	W64LIT(0xe5dac33f3f413fc3), W64LIT(0x92c71c5555ff551c),
	W64LIT(0x79db10a2a2fba210), W64LIT(0x03e965eaea23ea65),
	W64LIT(0x0f6aec6565af65ec), W64LIT(0xb90368babad3ba68),
	W64LIT(0x654a932f2f712f93), W64LIT(0x4e8ee7c0c05dc0e7),
	W64LIT(0xbe6081dede7fde81), W64LIT(0xe0fc6c1c1c241c6c),
	W64LIT(0xbb462efdfd1afd2e), W64LIT(0x521f644d4dd74d64),
	W64LIT(0xe476e09292ab92e0), W64LIT(0x8ffabc75759f75bc),
	W64LIT(0x30361e06060a061e), W64LIT(0x24ae988a8a838a98),
	W64LIT(0xf94b40b2b2cbb240), W64LIT(0x638559e6e637e659),
	W64LIT(0x707e360e0e120e36), W64LIT(0xf8e7631f1f211f63),
	W64LIT(0x3755f76262a662f7), W64LIT(0xee3aa3d4d461d4a3),
	W64LIT(0x298132a8a8e5a832), W64LIT(0xc452f49696a796f4),
	W64LIT(0x9b623af9f916f93a), W64LIT(0x66a3f6c5c552c5f6),
	W64LIT(0x3510b125256f25b1), W64LIT(0xf2ab205959eb5920),
	W64LIT(0x54d0ae84849184ae), W64LIT(0xb7c5a772729672a7),
	W64LIT(0xd5ecdd39394b39dd), W64LIT(0x5a16614c4cd44c61),
	W64LIT(0xca943b5e5ee25e3b), W64LIT(0xe79f857878887885),
	W64LIT(0xdde5d838384838d8), W64LIT(0x1498868c8c898c86),
	W64LIT(0xc617b2d1d16ed1b2), W64LIT(0x41e40ba5a5f2a50b),
	W64LIT(0x43a14de2e23be24d), W64LIT(0x2f4ef86161a361f8),
	W64LIT(0xf14245b3b3c8b345), W64LIT(0x1534a521216321a5),
	W64LIT(0x9408d69c9cb99cd6), W64LIT(0xf0ee661e1e221e66),
	W64LIT(0x2261524343c54352), W64LIT(0x76b1fcc7c754c7fc),
	W64LIT(0xb34f2bfcfc19fc2b), W64LIT(0x20241404040c0414),
	W64LIT(0xb2e3085151f35108), W64LIT(0xbc25c79999b699c7),
	W64LIT(0x4f22c46d6db76dc4), W64LIT(0x6865390d0d170d39),
	W64LIT(0x837935fafa13fa35), W64LIT(0xb66984dfdf7cdf84),
	W64LIT(0xd7a99b7e7e827e9b), W64LIT(0x3d19b424246c24b4),
	W64LIT(0xc5fed73b3b4d3bd7), W64LIT(0x319a3dababe0ab3d),
	W64LIT(0x3ef0d1cece4fced1), W64LIT(0x8899551111331155),
	W64LIT(0x0c83898f8f8c8f89), W64LIT(0x4a046b4e4ed24e6b),
	W64LIT(0xd16651b7b7c4b751), W64LIT(0x0be060ebeb20eb60),
	W64LIT(0xfdc1cc3c3c443ccc), W64LIT(0x7cfdbf81819e81bf),
	W64LIT(0xd440fe9494a194fe), W64LIT(0xeb1c0cf7f704f70c),
	W64LIT(0xa11867b9b9d6b967), W64LIT(0x988b5f131335135f),
	W64LIT(0x7d519c2c2c742c9c), W64LIT(0xd605b8d3d368d3b8),
	W64LIT(0x6b8c5ce7e734e75c), W64LIT(0x5739cb6e6eb26ecb),
	W64LIT(0x6eaaf3c4c451c4f3), W64LIT(0x181b0f030305030f),
	W64LIT(0x8adc135656fa5613), W64LIT(0x1a5e494444cc4449),
	W64LIT(0xdfa09e7f7f817f9e), W64LIT(0x218837a9a9e6a937),
	W64LIT(0x4d67822a2a7e2a82), W64LIT(0xb10a6dbbbbd0bb6d),
	W64LIT(0x4687e2c1c15ec1e2), W64LIT(0xa2f1025353f55302),
	W64LIT(0xae728bdcdc79dc8b), W64LIT(0x5853270b0b1d0b27),
	W64LIT(0x9c01d39d9dba9dd3), W64LIT(0x472bc16c6cb46cc1),
	W64LIT(0x95a4f531315331f5), W64LIT(0x87f3b974749c74b9),
	W64LIT(0xe31509f6f607f609), W64LIT(0x0a4c434646ca4643),
	W64LIT(0x09a526acace9ac26), W64LIT(0x3cb5978989868997),
	W64LIT(0xa0b44414143c1444), W64LIT(0x5bba42e1e13ee142),
	W64LIT(0xb0a64e16163a164e), W64LIT(0xcdf7d23a3a4e3ad2),
	W64LIT(0x6f06d06969bb69d0), W64LIT(0x48412d09091b092d),
	W64LIT(0xa7d7ad70709070ad), W64LIT(0xd96f54b6b6c7b654),
	W64LIT(0xce1eb7d0d06dd0b7), W64LIT(0x3bd67eeded2aed7e),
	W64LIT(0x2ee2dbcccc49ccdb), W64LIT(0x2a68574242c64257),
	W64LIT(0xb42cc29898b598c2), W64LIT(0x49ed0ea4a4f1a40e),
	W64LIT(0x5d75882828782888), W64LIT(0xda86315c5ce45c31),
	W64LIT(0x936b3ff8f815f83f), W64LIT(0x44c2a486869786a4),
};

static const word64 rc[R + 1] = {
	W64LIT(0x0000000000000000),
	W64LIT(0x1823c6e887b8014f),
	W64LIT(0x36a6d2f5796f9152),
	W64LIT(0x60bc9b8ea30c7b35),
	W64LIT(0x1de0d7c22e4bfe57),
	W64LIT(0x157737e59ff04ada),
	W64LIT(0x58c9290ab1a06b85),
	W64LIT(0xbd5d10f4cb3e0567),
	W64LIT(0xe427418ba77d95d8),
	W64LIT(0xfbee7c66dd17479e),
	W64LIT(0xca2dbf07ad5a8333)
};


// Whirlpool basic transformation. Transforms state based on block.
void Whirlpool::Transform(word64 *digest, const word64 *block)
{
	int r;
	word64 L[8];		// temporary storage
	word64 state[8];	// the cipher state
	word64 K[8];		// the round key

	// Compute and apply K^0 to the cipher state
	// Also apply part of the Miyaguchi-Preneel compression function
	digest[0] = state[0] = block[0] ^ (K[0] = digest[0]);
	digest[1] = state[1] = block[1] ^ (K[1] = digest[1]);
	digest[2] = state[2] = block[2] ^ (K[2] = digest[2]);
	digest[3] = state[3] = block[3] ^ (K[3] = digest[3]);
	digest[4] = state[4] = block[4] ^ (K[4] = digest[4]);
	digest[5] = state[5] = block[5] ^ (K[5] = digest[5]);
	digest[6] = state[6] = block[6] ^ (K[6] = digest[6]);
	digest[7] = state[7] = block[7] ^ (K[7] = digest[7]);

	// Iterate over all rounds:
	for (r = 1; r <= R; r++) {

		// Compute K^r from K^{r-1}:
		L[0] =
			C0[GETBYTE(K[4], 3)] ^
			C1[GETBYTE(K[3], 2)] ^
			C2[GETBYTE(K[2], 1)] ^
			C3[GETBYTE(K[1], 0)];
		L[0] = (L[0] >> 32) | (L[0] << 32);
		L[0] ^=
			C0[GETBYTE(K[0], 7)] ^
			C1[GETBYTE(K[7], 6)] ^
			C2[GETBYTE(K[6], 5)] ^
			C3[GETBYTE(K[5], 4)] ^
			rc[r];
		L[1] =
			C0[GETBYTE(K[5], 3)] ^
			C1[GETBYTE(K[4], 2)] ^
			C2[GETBYTE(K[3], 1)] ^
			C3[GETBYTE(K[2], 0)];
		L[1] = (L[1] >> 32) | (L[1] << 32);
		L[1] ^=
			C0[GETBYTE(K[1], 7)] ^
			C1[GETBYTE(K[0], 6)] ^
			C2[GETBYTE(K[7], 5)] ^
			C3[GETBYTE(K[6], 4)];
		L[2] =
			C0[GETBYTE(K[6], 3)] ^
			C1[GETBYTE(K[5], 2)] ^
			C2[GETBYTE(K[4], 1)] ^
			C3[GETBYTE(K[3], 0)];
		L[2] = (L[2] >> 32) | (L[2] << 32);
		L[2] ^=
			C0[GETBYTE(K[2], 7)] ^
			C1[GETBYTE(K[1], 6)] ^
			C2[GETBYTE(K[0], 5)] ^
			C3[GETBYTE(K[7], 4)];
		L[3] =
			C0[GETBYTE(K[7], 3)] ^
			C1[GETBYTE(K[6], 2)] ^
			C2[GETBYTE(K[5], 1)] ^
			C3[GETBYTE(K[4], 0)];
		L[3] = (L[3] >> 32) | (L[3] << 32);
		L[3] ^=
			C0[GETBYTE(K[3], 7)] ^
			C1[GETBYTE(K[2], 6)] ^
			C2[GETBYTE(K[1], 5)] ^
			C3[GETBYTE(K[0], 4)];
		L[4] =
			C0[GETBYTE(K[0], 3)] ^
			C1[GETBYTE(K[7], 2)] ^
			C2[GETBYTE(K[6], 1)] ^
			C3[GETBYTE(K[5], 0)];
		L[4] = (L[4] >> 32) | (L[4] << 32);
		L[4] ^=
			C0[GETBYTE(K[4], 7)] ^
			C1[GETBYTE(K[3], 6)] ^
			C2[GETBYTE(K[2], 5)] ^
			C3[GETBYTE(K[1], 4)];
		L[5] =
			C0[GETBYTE(K[1], 3)] ^
			C1[GETBYTE(K[0], 2)] ^
			C2[GETBYTE(K[7], 1)] ^
			C3[GETBYTE(K[6], 0)];
		L[5] = (L[5] >> 32) | (L[5] << 32);
		L[5] ^=
			C0[GETBYTE(K[5], 7)] ^
			C1[GETBYTE(K[4], 6)] ^
			C2[GETBYTE(K[3], 5)] ^
			C3[GETBYTE(K[2], 4)];
		L[6] =
			C0[GETBYTE(K[2], 3)] ^
			C1[GETBYTE(K[1], 2)] ^
			C2[GETBYTE(K[0], 1)] ^
			C3[GETBYTE(K[7], 0)];
		L[6] = (L[6] >> 32) | (L[6] << 32);
		L[6] ^=
			C0[GETBYTE(K[6], 7)] ^
			C1[GETBYTE(K[5], 6)] ^
			C2[GETBYTE(K[4], 5)] ^
			C3[GETBYTE(K[3], 4)];
		L[7] =
			C0[GETBYTE(K[3], 3)] ^
			C1[GETBYTE(K[2], 2)] ^
			C2[GETBYTE(K[1], 1)] ^
			C3[GETBYTE(K[0], 0)];
		L[7] = (L[7] >> 32) | (L[7] << 32);
		L[7] ^=
			C0[GETBYTE(K[7], 7)] ^
			C1[GETBYTE(K[6], 6)] ^
			C2[GETBYTE(K[5], 5)] ^
			C3[GETBYTE(K[4], 4)];
		K[0] = L[0];
		K[1] = L[1];
		K[2] = L[2];
		K[3] = L[3];
		K[4] = L[4];
		K[5] = L[5];
		K[6] = L[6];
		K[7] = L[7];

		// Apply the r-th round transformation:
		L[0] =
			C0[GETBYTE(state[4], 3)] ^
			C1[GETBYTE(state[3], 2)] ^
			C2[GETBYTE(state[2], 1)] ^
			C3[GETBYTE(state[1], 0)];
		L[0] = (L[0] >> 32) | (L[0] << 32);
		L[0] ^=
			C0[GETBYTE(state[0], 7)] ^
			C1[GETBYTE(state[7], 6)] ^
			C2[GETBYTE(state[6], 5)] ^
			C3[GETBYTE(state[5], 4)] ^
			K[0];
		L[1] =
			C0[GETBYTE(state[5], 3)] ^
			C1[GETBYTE(state[4], 2)] ^
			C2[GETBYTE(state[3], 1)] ^
			C3[GETBYTE(state[2], 0)];
		L[1] = (L[1] >> 32) | (L[1] << 32);
		L[1] ^=
			C0[GETBYTE(state[1], 7)] ^
			C1[GETBYTE(state[0], 6)] ^
			C2[GETBYTE(state[7], 5)] ^
			C3[GETBYTE(state[6], 4)] ^
			K[1];
		L[2] =
			C0[GETBYTE(state[6], 3)] ^
			C1[GETBYTE(state[5], 2)] ^
			C2[GETBYTE(state[4], 1)] ^
			C3[GETBYTE(state[3], 0)];
		L[2] = (L[2] >> 32) | (L[2] << 32);
		L[2] ^=
			C0[GETBYTE(state[2], 7)] ^
			C1[GETBYTE(state[1], 6)] ^
			C2[GETBYTE(state[0], 5)] ^
			C3[GETBYTE(state[7], 4)] ^
			K[2];
		L[3] =
			C0[GETBYTE(state[7], 3)] ^
			C1[GETBYTE(state[6], 2)] ^
			C2[GETBYTE(state[5], 1)] ^
			C3[GETBYTE(state[4], 0)];
		L[3] = (L[3] >> 32) | (L[3] << 32);
		L[3] ^=
			C0[GETBYTE(state[3], 7)] ^
			C1[GETBYTE(state[2], 6)] ^
			C2[GETBYTE(state[1], 5)] ^
			C3[GETBYTE(state[0], 4)] ^
			K[3];
		L[4] =
			C0[GETBYTE(state[0], 3)] ^
			C1[GETBYTE(state[7], 2)] ^
			C2[GETBYTE(state[6], 1)] ^
			C3[GETBYTE(state[5], 0)];
		L[4] = (L[4] >> 32) | (L[4] << 32);
		L[4] ^=
			C0[GETBYTE(state[4], 7)] ^
			C1[GETBYTE(state[3], 6)] ^
			C2[GETBYTE(state[2], 5)] ^
			C3[GETBYTE(state[1], 4)] ^
			K[4];
		L[5] =
			C0[GETBYTE(state[1], 3)] ^
			C1[GETBYTE(state[0], 2)] ^
			C2[GETBYTE(state[7], 1)] ^
			C3[GETBYTE(state[6], 0)];
		L[5] = (L[5] >> 32) | (L[5] << 32);
		L[5] ^=
			C0[GETBYTE(state[5], 7)] ^
			C1[GETBYTE(state[4], 6)] ^
			C2[GETBYTE(state[3], 5)] ^
			C3[GETBYTE(state[2], 4)] ^
			K[5];
		L[6] =
			C0[GETBYTE(state[2], 3)] ^
			C1[GETBYTE(state[1], 2)] ^
			C2[GETBYTE(state[0], 1)] ^
			C3[GETBYTE(state[7], 0)];
		L[6] = (L[6] >> 32) | (L[6] << 32);
		L[6] ^=
			C0[GETBYTE(state[6], 7)] ^
			C1[GETBYTE(state[5], 6)] ^
			C2[GETBYTE(state[4], 5)] ^
			C3[GETBYTE(state[3], 4)] ^
			K[6];
		L[7] =
			C0[GETBYTE(state[3], 3)] ^
			C1[GETBYTE(state[2], 2)] ^
			C2[GETBYTE(state[1], 1)] ^
			C3[GETBYTE(state[0], 0)];
		L[7] = (L[7] >> 32) | (L[7] << 32);
		L[7] ^=
			C0[GETBYTE(state[7], 7)] ^
			C1[GETBYTE(state[6], 6)] ^
			C2[GETBYTE(state[5], 5)] ^
			C3[GETBYTE(state[4], 4)] ^
			K[7];
		state[0] = L[0];
		state[1] = L[1];
		state[2] = L[2];
		state[3] = L[3];
		state[4] = L[4];
		state[5] = L[5];
		state[6] = L[6];
		state[7] = L[7];
	}

	// Apply the rest of the Miyaguchi-Preneel compression function:
	digest[0] ^= state[0];
	digest[1] ^= state[1];
	digest[2] ^= state[2];
	digest[3] ^= state[3];
	digest[4] ^= state[4];
	digest[5] ^= state[5];
	digest[6] ^= state[6];
	digest[7] ^= state[7];
}

NAMESPACE_END

#endif // WORD64_AVAILABLE
