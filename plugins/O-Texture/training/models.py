"""
O-Texture Neural Network Models
TextureEncoder, TextureDecoder, TextureVAE, TexturePrior, TexturePriorONNX
"""

import torch
import torch.nn as nn
import config


class TextureEncoder(nn.Module):
    """1D CNN encoder: audio[1, 4096] -> mu[32], logvar[32]"""

    def __init__(self, latent_dim=config.LATENT_DIM):
        super().__init__()

        layers = []
        channels = config.ENCODER_CHANNELS
        kernels = config.ENCODER_KERNELS
        strides = config.ENCODER_STRIDES

        for i in range(len(kernels)):
            padding = kernels[i] // 2
            layers.append(nn.Conv1d(channels[i], channels[i + 1],
                                    kernel_size=kernels[i], stride=strides[i],
                                    padding=padding))
            layers.append(nn.BatchNorm1d(channels[i + 1]))
            layers.append(nn.LeakyReLU(0.2, inplace=True))

        layers.append(nn.AdaptiveAvgPool1d(config.ENCODER_POOL_SIZE))

        self.conv_stack = nn.Sequential(*layers)

        flat_size = channels[-1] * config.ENCODER_POOL_SIZE
        self.fc_mu = nn.Linear(flat_size, latent_dim)
        self.fc_logvar = nn.Linear(flat_size, latent_dim)

    def forward(self, x):
        """
        Args:
            x: (batch, 1, 4096) audio waveform
        Returns:
            mu: (batch, 32)
            logvar: (batch, 32)
        """
        h = self.conv_stack(x)
        h = h.view(h.size(0), -1)
        return self.fc_mu(h), self.fc_logvar(h)


class TextureDecoder(nn.Module):
    """1D CNN decoder: z[32] -> audio[1, 4096]"""

    def __init__(self, latent_dim=config.LATENT_DIM):
        super().__init__()

        fc_out = config.DECODER_FC_OUT_CHANNELS * config.DECODER_FC_OUT_LENGTH
        self.fc = nn.Linear(latent_dim, fc_out)
        self.fc_channels = config.DECODER_FC_OUT_CHANNELS
        self.fc_length = config.DECODER_FC_OUT_LENGTH

        layers = []
        channels = config.DECODER_CHANNELS
        kernels = config.DECODER_KERNELS
        strides = config.DECODER_STRIDES

        for i in range(len(kernels)):
            padding = (kernels[i] - strides[i]) // 2
            layers.append(nn.ConvTranspose1d(channels[i], channels[i + 1],
                                              kernel_size=kernels[i],
                                              stride=strides[i],
                                              padding=padding))
            layers.append(nn.BatchNorm1d(channels[i + 1]))
            layers.append(nn.LeakyReLU(0.2, inplace=True))

        # Final conv to mono output
        layers.append(nn.Conv1d(channels[-1], 1,
                                kernel_size=config.DECODER_FINAL_KERNEL,
                                stride=1,
                                padding=config.DECODER_FINAL_KERNEL // 2))
        layers.append(nn.Tanh())

        self.conv_stack = nn.Sequential(*layers)

    def forward(self, z):
        """
        Args:
            z: (batch, 32) latent vector
        Returns:
            audio: (batch, 1, 4096) waveform
        """
        h = self.fc(z)
        h = h.view(h.size(0), self.fc_channels, self.fc_length)
        return self.conv_stack(h)


class TextureVAE(nn.Module):
    """Complete VAE: encoder + decoder with reparameterization trick."""

    def __init__(self, latent_dim=config.LATENT_DIM):
        super().__init__()
        self.encoder = TextureEncoder(latent_dim)
        self.decoder = TextureDecoder(latent_dim)

    def reparameterize(self, mu, logvar):
        std = torch.exp(0.5 * logvar)
        eps = torch.randn_like(std)
        return mu + eps * std

    def forward(self, x):
        """
        Args:
            x: (batch, 1, 4096) audio
        Returns:
            recon: (batch, 1, 4096) reconstructed audio
            mu: (batch, 32)
            logvar: (batch, 32)
        """
        mu, logvar = self.encoder(x)
        z = self.reparameterize(mu, logvar)
        recon = self.decoder(z)
        return recon, mu, logvar

    def generate(self, z):
        """Decode a latent vector to audio."""
        return self.decoder(z)


class TexturePrior(nn.Module):
    """GRU-based prior model for autoregressive latent sequence generation."""

    def __init__(self, latent_dim=config.LATENT_DIM,
                 hidden_dim=config.PRIOR_HIDDEN_DIM,
                 n_layers=config.PRIOR_NUM_LAYERS):
        super().__init__()
        self.gru = nn.GRU(latent_dim, hidden_dim, n_layers, batch_first=True)
        self.fc_mu = nn.Linear(hidden_dim, latent_dim)
        self.fc_logvar = nn.Linear(hidden_dim, latent_dim)

    def forward(self, z_sequence, hidden=None):
        """
        Teacher-forcing forward pass.
        Args:
            z_sequence: (batch, seq_len, 32)
            hidden: (n_layers, batch, hidden_dim) or None
        Returns:
            mu: (batch, seq_len, 32)
            logvar: (batch, seq_len, 32)
            hidden: (n_layers, batch, hidden_dim)
        """
        output, hidden = self.gru(z_sequence, hidden)
        mu = self.fc_mu(output)
        logvar = self.fc_logvar(output)
        return mu, logvar, hidden


class TexturePriorONNX(nn.Module):
    """ONNX-friendly wrapper with explicit hidden state I/O (single-step)."""

    def __init__(self, prior):
        super().__init__()
        self.gru = prior.gru
        self.fc_mu = prior.fc_mu
        self.fc_logvar = prior.fc_logvar

    def forward(self, z_input, hidden):
        """
        Single-step inference with explicit hidden state.
        Args:
            z_input: (1, 1, 32) single latent vector
            hidden: (2, 1, 128) GRU hidden state
        Returns:
            mu: (1, 32)
            logvar: (1, 32)
            hidden_out: (2, 1, 128) updated hidden state
        """
        output, hidden_out = self.gru(z_input, hidden)
        mu = self.fc_mu(output[:, -1, :])
        logvar = self.fc_logvar(output[:, -1, :])
        return mu, logvar, hidden_out
